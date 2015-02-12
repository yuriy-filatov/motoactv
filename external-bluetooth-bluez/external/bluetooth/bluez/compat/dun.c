/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2009-2010  Motorola Corporation
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#ifdef ANDROID_BLUETOOTHDUN
#include <glib.h>
#include <dbus/dbus.h>
#include <gdbus.h>
#endif

#include "dund.h"
#include "lib.h"

#define PROC_BASE  "/proc"

#ifdef ANDROID_BLUETOOTHDUN
#define DUN_PATH	"/org/bluez"
#define DUN_INTERFACE	"org.bluez.dun"
#define DUN_SERVICE	"org.bluez.dun"

#define DUN_TTY_TIMER_INTERVAL 500 /* check tty state every 500ms */

static DBusConnection *dbusConnection = NULL;
static struct DunConnection{
	int	control_io;

	GIOChannel	*dialup_io;
	int	dialup_sock;
	guint	dialup_watch;
	int	new_sock;
	guint	sock_watch;

	int	tty_id;
	char	tty[100];
	char	addr[40];

	guint	timer_id;/* Timeout id */
} dun_connection;

static int dun_create_tty(int sk, char *tty, int size);

static void emit_dun_session_connecting(char * addr)
{
	g_dbus_emit_signal(dbusConnection, DUN_PATH,
			DUN_INTERFACE, "DunConnecting",
			DBUS_TYPE_STRING, &addr,
			DBUS_TYPE_INVALID);
	syslog(LOG_INFO, "emit_dun_session_connecting, addr: %s", addr);
}

static void emit_dun_session_connected(char * path, char * addr)
{
	g_dbus_emit_signal(dbusConnection, DUN_PATH,
			DUN_INTERFACE, "DunConnected",
			DBUS_TYPE_STRING, &path,
			DBUS_TYPE_STRING, &addr,
			DBUS_TYPE_INVALID);
	syslog(LOG_INFO, "emit_dun_session_connected, path:%s, addr:%s", path, addr);
}

static void emit_dun_session_disconnected()
{
	g_dbus_emit_signal(dbusConnection, DUN_PATH,
			DUN_INTERFACE, "DunDisconnected",
			DBUS_TYPE_INVALID);
	syslog(LOG_INFO, "emit_dun_session_disconnected");
}

static gboolean dun_client_error(GIOChannel *chan, GIOCondition cond, gpointer data)
{
	syslog(LOG_INFO, "dun_client_error");

	if (data == &dun_connection) {
		if (dun_connection.new_sock >= 0) {
			/* The connecting end drop the connection before user accept */
			emit_dun_session_disconnected();
			dun_connection.new_sock = -1;
		}
	}
	return FALSE; /* return FALSE to automatically stop the monitoring */
}

static gboolean dun_tty_state_check(gpointer data)
{
	struct stat st;

	if (data == &dun_connection) {
		if (dun_connection.tty == NULL || stat(dun_connection.tty, &st) < 0) {
			/* tty file is removed */
			emit_dun_session_disconnected();
			dun_connection.tty[0] = 0;
		}
		else {
			return TRUE;
		}
	}

	dun_connection.timer_id = 0;
	return FALSE; /* return FALSE to automatically stop the timer */
}

static DBusMessage *dun_disconnect(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	struct stat st;
	struct rfcomm_dev_req req;
	int ret;

	syslog(LOG_INFO, "dun_disconnect, release the connection");
	if (data == &dun_connection) {
		/* user reject the incoming connection while tty is created */
		if (dun_connection.tty != NULL && stat(dun_connection.tty, &st) >= 0) {
			memset(&req, 0, sizeof(req));
			req.dev_id = dun_connection.tty_id;
			ret = ioctl(dun_connection.control_io, RFCOMMRELEASEDEV, &req);

			syslog(LOG_INFO, "dun_disconnect, return value: %d", ret);

			if (ret != 0) {
				return g_dbus_create_error(msg,
					"org.bluez.dun.Error.NotReleased",
					"Release Fail");
			}

			/* remove the file */
			unlink(dun_connection.tty);
			dun_connection.tty[0] = 0;
		}

		/* user reject the incoming connection before tty is created */
		if (dun_connection.new_sock >= 0) {
			/* Donot monitor the created socket as we will close the connection */
			g_source_remove(dun_connection.sock_watch);
			shutdown(dun_connection.new_sock, SHUT_RDWR);
			close(dun_connection.new_sock);
			dun_connection.new_sock = -1;
		}
		emit_dun_session_disconnected();
	}

	return dbus_message_new_method_return(msg);
}

static gboolean dun_create_device()
{
	gboolean ret = TRUE;

	dun_connection.tty_id = dun_create_tty(dun_connection.new_sock,
					dun_connection.tty,
					sizeof(dun_connection.tty) - 1);
	if (dun_connection.tty_id < 0) {
		syslog(LOG_ERR, "RFCOMM TTY creation failed. %s(%d)", strerror(errno), errno);
		ret = FALSE;
	} else {
		syslog(LOG_INFO, "New connection from %s", dun_connection.addr);
		syslog(LOG_INFO, "New created tty is :%s", dun_connection.tty);

		chmod(dun_connection.tty, 00666);
		emit_dun_session_connected(dun_connection.tty, dun_connection.addr);
		/* Start timer to monitor the tty state */
		dun_connection.timer_id = g_timeout_add(DUN_TTY_TIMER_INTERVAL,
						dun_tty_state_check,
						&dun_connection);
	}

	dun_connection.new_sock = -1;

	return ret;
}

static DBusMessage *dun_connect(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	syslog(LOG_INFO, "dun_connect, user confirm accept connection and create  device");
	if (data == &dun_connection) {
		if (dun_connection.new_sock < 0) {
			return g_dbus_create_error(msg, "org.bluez.dun.Error.AlreadyReleased",
							"Connect fails as already released");
		}

		/* Donot monitor the created socket while tty created */
		g_source_remove(dun_connection.sock_watch);
		if (dun_create_device() == FALSE) {
			return g_dbus_create_error(msg, "org.bluez.dun.Error.TTYNotCreated",
							"Fails to create TTY");
		}
	}

	return dbus_message_new_method_return(msg);
}

static gboolean dun_accept_event(GIOChannel *chan, GIOCondition cond, gpointer data)
{
	GIOChannel *io;
	int nsk;
	struct sockaddr_rc sa;

	syslog(LOG_INFO, "dun_accept_event, incoming connection, signal:%d", cond);
	if (cond & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
		g_io_channel_unref(chan);
		dun_connection.dialup_watch = 0;
		return FALSE;
	}

	if (data == &dun_connection) {
		socklen_t alen = sizeof(sa);

		nsk = accept(dun_connection.dialup_sock, (struct sockaddr *) &sa, &alen);
		if (dun_connection.new_sock >= 0 || dun_connection.timer_id > 0) {
			/* ONLY ACCEPT ONE CONNECTION */
			shutdown(nsk, SHUT_RDWR);
			close(nsk);
			return TRUE;
		}
	} else {
		dun_connection.dialup_watch = 0;
		return FALSE;
	}

	if (nsk < 0) {
		syslog(LOG_ERR, "Accept failed. %s(%d)", strerror(errno), errno);
		return TRUE;
	}

	dun_connection.new_sock = nsk;
	ba2str(&sa.rc_bdaddr, dun_connection.addr);

	/* monitor the new socket being released by the connectiong end */
	io = g_io_channel_unix_new(dun_connection.new_sock);
	dun_connection.sock_watch = g_io_add_watch_full(io, G_PRIORITY_DEFAULT,
			G_IO_HUP | G_IO_ERR | G_IO_NVAL,
			dun_client_error, &dun_connection, NULL);
	g_io_channel_unref(io);

	emit_dun_session_connecting(dun_connection.addr);

	return TRUE;
}

void dun_remove_watch()
{
	syslog(LOG_INFO, "dun_remove_watch, sock:%d, io:%d, watch:%d",
			dun_connection.dialup_sock, dun_connection.dialup_io,
			dun_connection.dialup_watch);

	if (dun_connection.timer_id) {
		g_source_remove(dun_connection.timer_id);
	}

	if (dun_connection.dialup_watch) {
		g_source_remove(dun_connection.dialup_watch);
		g_io_channel_unref(dun_connection.dialup_io);
	}
}

void dun_add_watch(int sk)
{
	dun_connection.dialup_sock = sk;
	dun_connection.dialup_io = g_io_channel_unix_new(dun_connection.dialup_sock);
	g_io_channel_set_close_on_unref(dun_connection.dialup_io, TRUE);

	dun_connection.dialup_watch = g_io_add_watch(dun_connection.dialup_io,
			G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
			dun_accept_event, &dun_connection);

	dun_connection.tty[0] = 0;
	dun_connection.tty_id = -1;
	dun_connection.timer_id = 0;
	dun_connection.new_sock = -1;

	syslog(LOG_INFO, "dun_add_watch, dun daemon is listening..., sock:%d, io:%d, watch:%d",
			dun_connection.dialup_sock, dun_connection.dialup_io,
			dun_connection.dialup_watch);
}

static GDBusMethodTable dun_methods[] = {
	{ "DunConnect",		"",	"",	dun_connect },
	{ "DunDisconnect",	"",	"",	dun_disconnect },
	{ }
};

static GDBusSignalTable dun_signals[] = {
	{ "DunConnecting", "s"  },
	{ "DunConnected", "ss"  },
	{ "DunDisconnected", ""  },
	{ }
};

int dun_dbus_init(void)
{
	DBusConnection *conn;
	int ctl_io;

	ctl_io = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_RFCOMM);
	if (ctl_io < 0) {
		return -1;
	}
	dun_connection.control_io = ctl_io;

	conn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, DUN_SERVICE, NULL);
	if (conn == NULL) {
		close(dun_connection.control_io);
		return -1;
	}

	if (g_dbus_register_interface(conn, DUN_PATH,
				 DUN_INTERFACE, dun_methods, dun_signals,
				 NULL, &dun_connection, NULL) == FALSE) {
		close(dun_connection.control_io);
		dbus_connection_unref(dbusConnection);
		dbusConnection = NULL;

		return -1;
	}

	dbusConnection = conn;

	return 0;
}

void dun_dbus_clean(void)
{
	close(dun_connection.control_io);

	g_dbus_unregister_interface(dbusConnection, DUN_PATH, DUN_INTERFACE);
	dbus_connection_unref(dbusConnection);
	dbusConnection = NULL;
}
#endif /* ANDROID_BLUETOOTHDUN */

static int for_each_port(int (*func)(struct rfcomm_dev_info *, unsigned long), unsigned long arg)
{
	struct rfcomm_dev_list_req *dl;
	struct rfcomm_dev_info *di;
	long r = 0;
	int  sk, i;

	sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_RFCOMM);
	if (sk < 0 ) {
		perror("Can't open RFCOMM control socket");
		exit(1);
	}

	dl = malloc(sizeof(*dl) + RFCOMM_MAX_DEV * sizeof(*di));
	if (!dl) {
		perror("Can't allocate request memory");
		close(sk);
		exit(1);
	}

	dl->dev_num = RFCOMM_MAX_DEV;
	di = dl->dev_info;

	if (ioctl(sk, RFCOMMGETDEVLIST, (void *) dl) < 0) {
		perror("Can't get device list");
		exit(1);
	}

	for (i = 0; i < dl->dev_num; i++) {
		r = func(di + i, arg);
		if (r) break;
	}

	close(sk);
	free(dl);
	return r;
}

static int uses_rfcomm(char *path, char *dev)
{
	struct dirent *de;
	DIR   *dir;

	dir = opendir(path);
	if (!dir)
		return 0;

	if (chdir(path) < 0)
		return 0;

	while ((de = readdir(dir)) != NULL) {
		char link[PATH_MAX + 1];
		int  len = readlink(de->d_name, link, sizeof(link));
		if (len > 0) {
			link[len] = 0;
			if (strstr(link, dev)) {
				closedir(dir);
				return 1;
			}
		}
	}

	closedir(dir);

	return 0;
}

static int find_pppd(int id, pid_t *pid)
{
	struct dirent *de;
	char  path[PATH_MAX + 1];
	char  dev[10];
	int   empty = 1;
	DIR   *dir;

	dir = opendir(PROC_BASE);
	if (!dir) {
		perror(PROC_BASE);
		return -1;
	}

	sprintf(dev, "rfcomm%d", id);

	*pid = 0;
	while ((de = readdir(dir)) != NULL) {
		empty = 0;
		if (isdigit(de->d_name[0])) {
			sprintf(path, "%s/%s/fd", PROC_BASE, de->d_name);
			if (uses_rfcomm(path, dev)) {
				*pid = atoi(de->d_name);
				break;
			}
		}
	}
	closedir(dir);

	if (empty)
		fprintf(stderr, "%s is empty (not mounted ?)\n", PROC_BASE);

	return *pid != 0;
}

static int dun_exec(char *tty, char *prog, char **args)
{
	int pid = fork();
	int fd;

	switch (pid) {
	case -1:
		return -1;

	case 0:
		break;

	default:
		return pid;
	}

	setsid();

	/* Close all FDs */
	for (fd = 3; fd < 20; fd++)
		close(fd);

	execvp(prog, args);

	syslog(LOG_ERR, "Error while executing %s", prog);

	exit(1);
}

static int dun_create_tty(int sk, char *tty, int size)
{
	struct sockaddr_rc sa;
	struct stat st;
	socklen_t alen;
	int id, try = 30;

	struct rfcomm_dev_req req = {
		flags:   (1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP),
		dev_id:  -1
	};

	alen = sizeof(sa);
	if (getpeername(sk, (struct sockaddr *) &sa, &alen) < 0)
		return -1;
	bacpy(&req.dst, &sa.rc_bdaddr);

	alen = sizeof(sa);
	if (getsockname(sk, (struct sockaddr *) &sa, &alen) < 0)
		return -1;
	bacpy(&req.src, &sa.rc_bdaddr);
	req.channel = sa.rc_channel;

	id = ioctl(sk, RFCOMMCREATEDEV, &req);
	if (id < 0)
		return id;

	snprintf(tty, size, "/dev/rfcomm%d", id);
	while (stat(tty, &st) < 0) {
		snprintf(tty, size, "/dev/bluetooth/rfcomm/%d", id);
		if (stat(tty, &st) < 0) {
			snprintf(tty, size, "/dev/rfcomm%d", id);
			if (try--) {
				usleep(100 * 1000);
				continue;
			}

			memset(&req, 0, sizeof(req));
			req.dev_id = id;
			ioctl(sk, RFCOMMRELEASEDEV, &req);

			return -1;
		}
	}

	return id;
}

int dun_init(void)
{
	return 0;
}

int dun_cleanup(void)
{
	return 0;
}

static int show_conn(struct rfcomm_dev_info *di, unsigned long arg)
{
	pid_t pid;

	if (di->state == BT_CONNECTED &&
		(di->flags & (1<<RFCOMM_REUSE_DLC)) &&
		(di->flags & (1<<RFCOMM_TTY_ATTACHED)) &&
		(di->flags & (1<<RFCOMM_RELEASE_ONHUP))) {

		if (find_pppd(di->id, &pid)) {
			char dst[18];
			ba2str(&di->dst, dst);

			printf("rfcomm%d: %s channel %d pppd pid %d\n",
					di->id, dst, di->channel, pid);
		}
	}
	return 0;
}

static int kill_conn(struct rfcomm_dev_info *di, unsigned long arg)
{
	bdaddr_t *dst = (bdaddr_t *) arg;
	pid_t pid;

	if (di->state == BT_CONNECTED &&
		(di->flags & (1<<RFCOMM_REUSE_DLC)) &&
		(di->flags & (1<<RFCOMM_TTY_ATTACHED)) &&
		(di->flags & (1<<RFCOMM_RELEASE_ONHUP))) {

		if (dst && bacmp(&di->dst, dst))
			return 0;

		if (find_pppd(di->id, &pid)) {
			if (kill(pid, SIGINT) < 0)
				perror("Kill");

			if (!dst)
				return 0;
			return 1;
		}
	}
	return 0;
}

int dun_show_connections(void)
{
	for_each_port(show_conn, 0);
	return 0;
}

int dun_kill_connection(uint8_t *dst)
{
	for_each_port(kill_conn, (unsigned long) dst);
	return 0;
}

int dun_kill_all_connections(void)
{
	for_each_port(kill_conn, 0);
	return 0;
}

int dun_open_connection(int sk, char *pppd, char **args, int wait)
{
	char tty[100];
	int  pid;

	if (dun_create_tty(sk, tty, sizeof(tty) - 1) < 0) {
		syslog(LOG_ERR, "RFCOMM TTY creation failed. %s(%d)", strerror(errno), errno);
		return -1;
	}

	args[0] = "pppd";
	args[1] = tty;
	args[2] = "nodetach";

	pid = dun_exec(tty, pppd, args);
	if (pid < 0) {
		syslog(LOG_ERR, "Exec failed. %s(%d)", strerror(errno), errno);
		return -1;
	}

	if (wait) {
		int status;
		waitpid(pid, &status, 0);
		/* FIXME: Check for waitpid errors */
	}

	return 0;
}
