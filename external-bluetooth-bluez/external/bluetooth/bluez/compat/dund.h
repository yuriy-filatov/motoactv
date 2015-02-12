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

#define DUN_CONFIG_DIR	"/etc/bluetooth/dun"

#define DUN_DEFAULT_CHANNEL	1

#define DUN_MAX_PPP_OPTS	40

int dun_init(void);
int dun_cleanup(void);

int dun_show_connections(void);
int dun_kill_connection(uint8_t *dst);
int dun_kill_all_connections(void);

int dun_open_connection(int sk, char *pppd, char **pppd_opts, int wait);

int ms_dun(int fd, int server, int timeo);

/* DBUS functions */
#ifdef ANDROID_BLUETOOTHDUN
void dun_add_watch(int sk);
void dun_remove_watch();
int  dun_dbus_init(void);
void dun_dbus_clean(void);
#endif