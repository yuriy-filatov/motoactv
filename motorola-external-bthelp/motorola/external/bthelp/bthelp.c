/*
 *  Utility functions to support Motorola Bluetooth test commands.
 *
 *  Copyright (C) 2009 Motorola, Inc.
 *  Most of the code is derived from BlueZ 4.40.
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
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <pthread.h>
#include <termios.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <cutils/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>

#include <hciattach.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "bthelp"

/* DO NOT define DEBUG_TO_IO
  hci_cmd depends on IO output */
#define DEBUG_TO_ADB

#ifdef DEBUG_TO_ADB
#define DBG(fmt, arg...) LOGD("%s(): " fmt, __FUNCTION__, ##arg)
#define ERR(fmt, arg...) LOGE("%s(): " fmt, __FUNCTION__, ##arg)
#elif defined(DEBUG_TO_IO)
#define DBG(fmt, arg...) printf(fmt, ##arg)
#define ERR(fmt, arg...) printf(fmt, ##arg)
#else
#define DBG(fmt, arg...)
#define ERR(fmt, arg...)
#endif

#define UART_PORT "/dev/ttyS1"

static int hci_cmd_timeout = 0, hci_cmd_len = 0;
static unsigned char hci_buf[HCI_MAX_EVENT_SIZE];

static void *l2ping(void *arg);
static void *l2test(void *arg);

static int l2ping_result;
static int l2test_result;

static int terminate;

static void sig_term(int sig)
{
	terminate = 1;
}

/* Light-weight implementation of hcid */
static int hcid(char *pin_code, int testcase, void *arg)
{
	int sock;
	struct hci_filter flt;
	struct sockaddr_hci addr;
	struct pollfd fds[1];
	int len;
	unsigned char buf[HCI_MAX_EVENT_SIZE];
	int i;
	int n;
	struct sigaction sa;
	int err = -1;
	struct hci_dev_list_req dl[1];
	int dd;
	struct hci_dev_req dr;
	unsigned char pin_code_reply_param[PIN_CODE_REPLY_CP_SIZE];
	unsigned int pin_code_len = strlen(pin_code);
	pthread_t thread;
	pthread_attr_t attr;
	int thread_return;
	int received_linkkey = 0;
	write_class_of_dev_cp class_cp;
	change_local_name_cp name_cp;

	/* Create and bind HCI socket */
	sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (sock < 0) {
		ERR("Can't open HCI socket: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup;
	}

	memset(&addr, 0, sizeof(addr));
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = HCI_DEV_NONE;
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		ERR("Can't bind HCI socket: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup2;
	}

	/* Done creating and binding socket */

	/* Retrieve device */

	dl->dev_num = 1;

	if (ioctl(sock, HCIGETDEVLIST, (void *)dl) < 0) {
		ERR("Can't get device list: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup2;
	}

	/* Done retrieving device */

	/* Initialize device */

	dd = hci_open_dev(dl->dev_req->dev_id);
	if (dd < 0) {
		ERR("Can't open device: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup2;
	}

	memset(&dr, 0, sizeof(dr));
	dr.dev_id = dl->dev_req->dev_id;

	/* Set link mode */
	dr.dev_opt = HCI_LM_ACCEPT;
	if (ioctl(dd, HCISETLINKMODE, (unsigned long)&dr) < 0) {
		ERR("Can't set link mode: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup3;
	}

	/* Start HCI device */
	if (ioctl(dd, HCIDEVUP, dl->dev_req->dev_id) < 0 && errno != EALREADY) {
		ERR("Can't init device: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup3;
	}


	fds[0].fd = dd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	terminate = 0;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	/* Setup filter */
	hci_filter_clear(&flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
	hci_filter_all_events(&flt);
	/*
	 *hci_filter_set_event(EVT_PIN_CODE_REQ, &flt);
	 *EVT_LINK_KEY_REQ
	 *EVT_USER_CONFIRM_REQUEST
	 *EVT_USER_PASSKEY_REQUEST
	 */
	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
		ERR("Can't set filter: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup3;
	}
	/* test case hci_cmd */
	if (testcase == 3) {
		unsigned char *ptr = hci_buf;

	        uint16_t ocf;
	        uint8_t ogf;
		uint16_t opcode;
	        int plen;
	        void * params = NULL;
		hci_event_hdr *hdr;
#if 0
		for (i = 0; (unsigned long)i < hci_cmd_len; i++) {
			printf("%c", ptr[i]);
		}
#endif
		opcode = *(ptr + 1) + (*(ptr +2) << 8);
		ogf = cmd_opcode_ogf(opcode);
		ocf = cmd_opcode_ocf(opcode);
		plen = *(ptr + 3);
		if (plen > 0)
			params = ptr + 4;

		DBG("opcode = 0x%04X\n", opcode);
		DBG("ogf = 0x%02X\n", ogf);
		DBG("ocf = 0x%04X\n", ocf);
		DBG("plen = %d\n", plen);

		if (hci_send_cmd(dd, ogf, ocf, plen, params) < 0) {
			ERR("Can't send_cmd: %s: errno=%d\n",
				strerror(errno), errno);
			err = errno;
			goto cleanup3;
	        }

		len = read(dd, buf, sizeof(buf));

		if (len < 0) {
			ERR("Can't read: %s: errno=%d\n",
				strerror(errno), errno);
			err = errno;
			goto cleanup3;
		}

		for (i = 0; i < len; i++) {
			printf("%c", buf[i]);
			DBG("0x%02X", buf[i]);
		}
		err = 0;
		/* finish test case */
		goto cleanup3;
	}

	/* Set link policy */
	dr.dev_opt = HCI_LP_RSWITCH | HCI_LP_SNIFF | HCI_LP_HOLD | HCI_LP_PARK;
	if ((ioctl(dd, HCISETLINKPOL, (unsigned long)&dr) < 0)
	    && (errno != ENETDOWN)) {
		ERR("Can't set link policy: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup3;
	}

	/* Set device name */
	memset(&name_cp, 0, sizeof(name_cp));
	memcpy(name_cp.name, "Moto BT TCMD", 12);
	hci_send_cmd(dd, OGF_HOST_CTL, OCF_CHANGE_LOCAL_NAME,
		     CHANGE_LOCAL_NAME_CP_SIZE, &name_cp);

	/* Set device class */
	memset(&class_cp, 0, sizeof(class_cp));
	class_cp.dev_class[0] = 0x0C;	/* Smart phone minor class */
	class_cp.dev_class[1] = 0x02;	/* Phone major class */
	class_cp.dev_class[2] = 0x7E;	/* Fake features */
	hci_send_cmd(dd, OGF_HOST_CTL, OCF_WRITE_CLASS_OF_DEV,
		     WRITE_CLASS_OF_DEV_CP_SIZE, &class_cp);

	/* Done setting up BT daemon */

	/* Perform action */
	if (testcase == 1) {
		l2test_result = -1;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		err = pthread_create(&thread, &attr, l2test, (void *)arg);
		if (err != 0) {
			ERR("Failed to create l2test thread\n");
			err = -1;
			goto cleanup3;
		}
	} else if (testcase == 2) {
		l2ping_result = -1;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		err = pthread_create(&thread, &attr, l2ping, (void *)arg);
		if (err != 0) {
			ERR("Failed to create l2ping thread\n");
			err = -1;
			goto cleanup3;
		}
	}

	while (!terminate) {
		n = poll(fds, 1, 250);
		if (n <= 0)
			continue;
		if (fds[0].revents & (POLLHUP | POLLERR | POLLNVAL)) {
			ERR("device disconnected\n");
			err = 0;
			goto cleanup3;
		}
		len = read(fds[0].fd, buf, sizeof(buf));
		if (len < 0) {
			ERR("Read failed: %s: errno=%d\n",
				strerror(errno), errno);
			err = errno;
			goto cleanup3;
		}
#if 0
		/* For debugging only */
		printf("mypasskey: %02X %02X %02X ", buf[0], buf[1], buf[2]);
		for (i = 0; i < buf[2]; i++) {
			printf("%02X ", buf[3 + i]);
		}
		printf("\n");
#endif

		if ((buf[1] == 0x17) && (buf[2] == 0x06)) {
			/* If link_key_request event is received,
			 * then respond using link_key_request_negative_reply
			 */
			hci_send_cmd(fds[0].fd, OGF_LINK_CTL,
				     OCF_LINK_KEY_NEG_REPLY, 6, &buf[3]);
		} else if ((buf[1] == 0x16) && (buf[2] == 0x06)) {
			/* If pin_code_request event is received,
			 * then respond using pin_code_request_reply
			 */
			memset(pin_code_reply_param, 0,
			       sizeof(pin_code_reply_param));
			memcpy(pin_code_reply_param, &buf[3], 6);
			memcpy(pin_code_reply_param + 6, &pin_code_len, 1);
			if (pin_code_len + 6 + 1 <= sizeof(pin_code_reply_param)) {
				memcpy(pin_code_reply_param + 6 + 1, pin_code,
					pin_code_len);
			} else {
				ERR("pin_code length (%d) is greater than allowed.\n",
					pin_code_len);
				err = -1;
				goto cleanup3;
			}
			hci_send_cmd(fds[0].fd, OGF_LINK_CTL,
				     OCF_PIN_CODE_REPLY, PIN_CODE_REPLY_CP_SIZE,
				     pin_code_reply_param);
		} else if (buf[1] == 0x18) {
			/* If link_key_notification event is received,
			 * then signal this event.
			 */
			received_linkkey = 1;
		}
	}

	err = 0;

	/* Evaluate test result */
	if ((testcase == 1) && (!received_linkkey)) {
		ERR("received_linkkey=%d\n", received_linkkey);
		ERR("l2test_result=%d\n", l2test_result);
		err = l2test_result;
	} else if ((testcase == 2) && (l2ping_result != 0)) {
		ERR("l2ping_result=%d\n", l2ping_result);
		err = l2ping_result;
	}

      cleanup3:
	hci_close_dev(dd);
      cleanup2:
	close(sock);
      cleanup:
	return err;
}

/* End of hcid implementation */

/* Functions implementing hciattach */
static int uart_speed(int s)
{
	switch (s) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
#ifdef B2500000
	case 2500000:
		return B2500000;
#endif
#ifdef B3000000
	case 3000000:
		return B3000000;
#endif
#ifdef B3500000
	case 3500000:
		return B3500000;
#endif
#ifdef B4000000
	case 4000000:
		return B4000000;
#endif
	default:
		return B57600;
	}
}

int set_speed(int fd, struct termios *ti, int speed)
{
	cfsetospeed(ti, uart_speed(speed));
	cfsetispeed(ti, uart_speed(speed));
	return tcsetattr(fd, TCSANOW, ti);
}

int read_hci_event(int fd, unsigned char *buf, int size)
{
	int remain, r;
	int count = 0;

	if (size <= 0)
		return -1;

	/* The first byte identifies the packet type. For HCI event packets, it
	 * should be 0x04, so we read until we get to the 0x04. */
	while (1) {
		r = read(fd, buf, 1);
		if (r <= 0)
			return -1;
		if (buf[0] == 0x04)
			break;
	}
	count++;

	/* The next two bytes are the event code and parameter total length. */
	while (count < 3) {
		r = read(fd, buf + count, 3 - count);
		if (r <= 0)
			return -1;
		count += r;
	}

	/* Now we read the parameters. */
	if (buf[2] < (size - 3))
		remain = buf[2];
	else
		remain = size - 3;

	while ((count - 3) < remain) {
		r = read(fd, buf + count, remain - (count - 3));
		if (r <= 0)
			return -1;
		count += r;
	}

	return count;
}

typedef struct {
	uint8_t uart_prefix;
	hci_event_hdr hci_hdr;
	evt_cmd_complete cmd_complete;
	uint8_t status;
	uint8_t data[16];
} __attribute__ ((packed)) command_complete_t;

static int read_command_complete(int fd, unsigned short opcode,
				 unsigned char len)
{
	command_complete_t resp;
	/* Read reply. */
	if (read_hci_event(fd, (unsigned char *)&resp, sizeof(resp)) < 0) {
		ERR("Failed to read response\n");
		return -1;
	}
	/* Parse speed-change reply */
	if (resp.uart_prefix != HCI_EVENT_PKT) {
		ERR("Error in response: not an event packet, but 0x%02x!\n",
			resp.uart_prefix);
		return -1;
	}

	if (resp.hci_hdr.evt != EVT_CMD_COMPLETE) {	/* event must be event-complete */
		ERR("Error in response: not a cmd-complete event, but 0x%02x!\n",
			resp.hci_hdr.evt);
		return -1;
	}

	if (resp.hci_hdr.plen < 4) {	/* plen >= 4 for EVT_CMD_COMPLETE */
		ERR("Error in response: plen is not >= 4, but 0x%02x!\n",
			resp.hci_hdr.plen);
		return -1;
	}

	/* cmd-complete event: opcode */
	if (resp.cmd_complete.opcode != (uint16_t) opcode) {
		ERR("Error in response: opcode is 0x%04x, not 0x%04x!\n",
			resp.cmd_complete.opcode, opcode);
		return -1;
	}

	return resp.status == 0 ? 0 : -1;
}

typedef struct {
	uint8_t uart_prefix;
	hci_command_hdr hci_hdr;
	uint32_t speed;
} __attribute__ ((packed)) texas_speed_change_cmd_t;

static int texas_change_speed(int fd, struct termios *ti, uint32_t speed)
{
	/* Send a speed-change request */
	texas_speed_change_cmd_t cmd;
	int n;

	char lcmd[4];
	unsigned char resp[100];	/* Response */

	DBG("Enter texas_change_speed.\n");
	/* Get Manufacturer and LMP version */
	lcmd[0] = HCI_COMMAND_PKT;
	lcmd[1] = 0x01;
	lcmd[2] = 0x10;
	lcmd[3] = 0x00;

	do {
		n = write(fd, lcmd, 4);
		if (n < 0) {
			ERR("Failed to write init command");
			return -1;
		}
		if (n < 4) {
			ERR("Wanted to write 4 bytes, could only write %d. Stop\n", n);
			return -1;
		}

		/* Read reply. */
		if (read_hci_event(fd, resp, 100) < 0) {
			ERR("Failed to read init response");
			return -1;
		}

		/* Wait for command complete event for our Opcode */
	} while (resp[4] != lcmd[1] && resp[5] != lcmd[2]);

	/* Verify manufacturer */
	if ((resp[11] & 0xFF) != 0x0d)
		ERR("WARNING : module's manufacturer is not Texas Instrument\n");

	/* Print LMP version */
	DBG("Texas module LMP version : 0x%02x\n", resp[10] & 0xFF);

	cmd.uart_prefix = HCI_COMMAND_PKT;
	cmd.hci_hdr.opcode = 0xff36;
	cmd.hci_hdr.plen = sizeof(uint32_t);
	cmd.speed = speed;

	n = write(fd, &cmd, sizeof(cmd));

	if (n < 0) {
		ERR("Failed to write: %s: errno=%d\n",
			strerror(errno), errno);
		return -1;
	}

	if (n < (int)sizeof(cmd)) {
		ERR("Wanted to write %d bytes, could only write %d\n",
			(int)sizeof(cmd), n);
		return -1;
	}

	/* Parse speed-change reply */
	if (read_command_complete(fd, cmd.hci_hdr.opcode, cmd.hci_hdr.plen) < 0) {
		ERR("Failed to read\n");
		return -1;
	}

	DBG("texas_change_speed read_command_complete ok\n");

	if (set_speed(fd, ti, speed) < 0) {
		ERR("Can't set baud rate: %s: errno=%d\n",
			strerror(errno), errno);
		return -1;
	}

	return 0;
}

static int texas_load_firmware(int fd, const char *firmware, char *sleepmode)
{

	int fw = open(firmware, O_RDONLY);

	DBG("Opening firmware file: %s\n", firmware);

	if (fw < 0) {
		ERR("Could not open firmware file %s: %s (%d).\n",
			firmware, strerror(errno), errno);
		return -1;
	}

	DBG("Uploading firmware...\n");
	do {
		/* Read each command and wait for a response. */
		unsigned char data[1024];
		unsigned char cmdp[1 + sizeof(hci_command_hdr)];
		hci_command_hdr *cmd = (hci_command_hdr *) (cmdp + 1);
		int nr;
		nr = read(fw, cmdp, sizeof(cmdp));
		if (!nr)
			break;

		if (nr != sizeof(cmdp)) {
			ERR("Could not read H4 + HCI header!\n");
			return -1;
		}
		if (*cmdp != HCI_COMMAND_PKT) {
			ERR("Command is not an H4 command packet!\n");
			return -1;
		}

		if (read(fw, data, cmd->plen) != cmd->plen) {
			ERR("Could not read %d bytes of data for command with opcode %04x!\n",
				cmd->plen, cmd->opcode);
			return -1;
		}

		{
			int nw;
#if 0
			DBG("\topcode 0x%04x (%d bytes of data).\n",
				cmd->opcode, cmd->plen);
#endif
			struct iovec iov_cmd[2];
			iov_cmd[0].iov_base = cmdp;
			iov_cmd[0].iov_len = sizeof(cmdp);
			iov_cmd[1].iov_base = data;
			iov_cmd[1].iov_len = cmd->plen;
			nw = writev(fd, iov_cmd, 2);
			if (nw != (int)sizeof(cmd) + cmd->plen) {
				ERR("Could not send full command (sent only %d bytes)!\n",
					nw);
				return -1;
			}
		}

		/* Wait for response */
		if (read_command_complete(fd, cmd->opcode, cmd->plen) < 0) {
			return -1;
		}

	} while (1);
	DBG("Firmware upload successful.\n");

	/* Configure sleep mode for test command; default is "disable" */
	{
		unsigned char cmdp[1 + sizeof(hci_command_hdr)];
		hci_command_hdr *cmd = (hci_command_hdr *) (cmdp + 1);
		unsigned char data[9] =
		    { 0x00, 0x00, 0x00, 0x05, 0xFF, 0xFF, 0x02, 0x64, 0x00 };
		int nw;
		struct iovec iov_cmd[2];

		if (!strcmp(sleepmode, "6wire")) {
			data[0] = 0x01;
			data[1] = 0x01;
			data[2] = 0x03;
		} else if (!strcmp(sleepmode, "disable")) {
			data[0] = 0x00;
			data[1] = 0x00;
			data[2] = 0x00;
		} else if (!strcmp(sleepmode, "hcill")) {
			data[0] = 0x01;
			data[1] = 0x01;
			data[2] = 0x00;
		}

		cmdp[0] = HCI_COMMAND_PKT;
		cmd->opcode = 0xFD0C;
		cmd->plen = 0x09;

		iov_cmd[0].iov_base = cmdp;
		iov_cmd[0].iov_len = sizeof(cmdp);
		iov_cmd[1].iov_base = data;
		iov_cmd[1].iov_len = cmd->plen;
		nw = writev(fd, iov_cmd, 2);
		if (nw != (int)sizeof(cmd) + cmd->plen) {
			ERR("Could not send entire command (sent only %d bytes)!\n",
				nw);
			return -1;
		}
		if (read_command_complete(fd, cmd->opcode, cmd->plen) < 0) {
			return -1;
		}
		DBG("Set sleep mode %s successful.\n", sleepmode);
	}

	close(fw);
	return 0;
}

static int download_firmware(int fd, int speed, struct termios *ti,
			     char *firmware, char *sleepmode)
{
	int err;

	err = texas_change_speed(fd, ti, speed);
	if (err) {
		ERR("Failed to change speed: err=%d\n",
			err);
		goto cleanup;
	}
	err = texas_load_firmware(fd, firmware, sleepmode);
	if (err) {
		ERR("Failed to load firmware: err=%d\n",
			err);
	}

      cleanup:
	return err;
}

static int hciattach_start(char *dev, int speed, char *firmware,
			   char *sleepmode)
{
	int fd;
	struct termios ti;
	int i;
	int err;

	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		ERR("Can't open serial port: %s: errno=%d\n",
			strerror(errno), errno);
		err = -1;
		goto cleanup;
	}

	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0) {
		ERR("Can't get port settings: %s: errno=%d\n",
			strerror(errno), errno);
		err = -1;
		goto cleanup2;
	}

	cfmakeraw(&ti);
	ti.c_cflag |= CLOCAL;
	ti.c_cflag |= CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		ERR("Can't set port settings: %s: errno=%d\n",
			strerror(errno), errno);
		err = -1;
		goto cleanup2;
	}

	if (firmware != NULL) {
		if (set_speed(fd, &ti, 115200) < 0) {
			ERR("Can't set initial baud rate: %s: errno=%d\n",
				strerror(errno), errno);
			err = -1;
			goto cleanup2;
		}

		tcflush(fd, TCIOFLUSH);

		err = download_firmware(fd, speed, &ti, firmware, sleepmode);
		if (err) {
			ERR("Failed firmware download: err=%d\n",
				err);
			err = -1;
			goto cleanup2;
		}
	}

	tcflush(fd, TCIOFLUSH);

	/* Set actual baudrate */
	if (set_speed(fd, &ti, speed) < 0) {
		ERR("Can't set baud rate: %s: errno=%d\n",
			strerror(errno), errno);
		err = -1;
		goto cleanup2;
	}

	/* Set TTY to N_HCI line discipline */
	i = N_HCI;
	if (ioctl(fd, TIOCSETD, &i) < 0) {
		ERR("Can't set line discipline: %s: errno=%d\n",
			strerror(errno), errno);
		err = -1;
		goto cleanup2;
	}

	if (ioctl(fd, HCIUARTSETPROTO, HCI_UART_H4) < 0) {
		ERR("Can't set protocol: %s: errno=%d\n",
			strerror(errno), errno);
		err = -1;
		goto cleanup2;
	}

	return fd;

      cleanup2:
	close(fd);
      cleanup:
	return err;
}

static int hciattach_stop(int fd)
{
	int ld;

	/* Restore TTY line discipline */
	ld = N_TTY;
	if (ioctl(fd, TIOCSETD, &ld) < 0) {
		ERR("Can't restore line discipline: %s: errno=%d\n",
			strerror(errno), errno);
		return -1;
	}

	return 0;
}

/* End of hciattach implementation */


static void *l2ping(void *arg)
{
	int err;
	struct sockaddr_l2 addr;
	unsigned char send_buf[L2CAP_CMD_HDR_SIZE + 44];
	unsigned char recv_buf[L2CAP_CMD_HDR_SIZE + 44];
	int i, sk;
	uint8_t id = 200;
	int size = 44;
	l2cap_cmd_hdr *send_cmd = (l2cap_cmd_hdr *) send_buf;
	l2cap_cmd_hdr *recv_cmd = (l2cap_cmd_hdr *) recv_buf;
	struct pollfd pf[1];
	struct hci_conn_info_req *cr;
	int dd;

	/* Create socket */
	sk = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP);
	if (sk < 0) {
		ERR("Can't create socket : %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup;
	}

	/* Bind to local address */
	memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, BDADDR_ANY);

	if (bind(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		ERR("Can't bind socket: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup2;
	}

	/* Connect to remote device */
	memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	str2ba((char *)arg, &addr.l2_bdaddr);

	if (connect(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		ERR("Can't connect: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup3;
	}

	/* Initialize send buffer */
	for (i = 0; i < size; i++)
		send_buf[L2CAP_CMD_HDR_SIZE + i] = (i % 40) + 'A';

	/* Build command header */
	send_cmd->ident = id;
	send_cmd->len = htobs(size);
	send_cmd->code = L2CAP_ECHO_REQ;

	/* Send Echo Command */
	if (send(sk, send_buf, L2CAP_CMD_HDR_SIZE + size, 0) <= 0) {
		ERR("Send failed: %s: errno=%d\n", strerror(errno), errno);
		err = errno;
		goto cleanup4;
	}

	/* Wait for Echo Response */

	pf[0].fd = sk;
	pf[0].events = POLLIN;

	if ((err = poll(pf, 1, 5 * 1000)) < 0) {
		ERR("Poll failed: %s: errno=%d\n", strerror(errno), errno);
		err = errno;
		goto cleanup4;
	}

	if (!err) {
		ERR("Timed out waiting for echo response\n");
		err = -1;
		goto cleanup4;
	}

	if (pf[0].revents & (POLLHUP | POLLERR | POLLNVAL)) {
		ERR("Disconnected\n");
		err = -1;
		goto cleanup4;
	}

	if ((err = recv(sk, recv_buf, L2CAP_CMD_HDR_SIZE + size, 0)) < 0) {
		ERR("Recv failed: %s: errno=%d\n", strerror(errno), errno);
		err = errno;
		goto cleanup4;
	}

	if (!err) {
		ERR("Disconnected\n");
		err = -1;
		goto cleanup4;
	}

	recv_cmd->len = btohs(recv_cmd->len);

	/* Check for our id */
	if (recv_cmd->ident != id) {
		ERR("ID mismatch\n");
		err = -1;
		goto cleanup4;
	}

	/* Check type */
	if (recv_cmd->code == L2CAP_COMMAND_REJ) {
		ERR("Peer doesn't support Echo packets\n");
		err = -1;
		goto cleanup4;
	}

	if (recv_cmd->code != L2CAP_ECHO_RSP) {
		ERR("Opcode mismatch\n");
		err = -1;
		goto cleanup4;
	}

	err = 0;

      cleanup4:
	shutdown(sk, SHUT_RDWR);

      cleanup3:
	/* Try to explicitly disconnect baseband connection
	 * If encounter failure, then sleep for slightly longer
	 * than 2 seconds to allow baseband connection to timeout
	 * and self-terminate.
	 */
	dd = hci_open_dev(0);
	if (dd < 0) {
		ERR("HCI device open failed: %s: errno=%d\n",
			strerror(errno), errno);
		usleep(2100000);
		goto cleanup2;
	}

	cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
	if (!cr) {
		ERR("Can't allocate memory: %s: errno=%d\n",
			strerror(errno), errno);
		hci_close_dev(dd);
		usleep(2100000);
		goto cleanup2;
	}

	str2ba((char *)arg, &cr->bdaddr);
	cr->type = ACL_LINK;

	if (ioctl(dd, HCIGETCONNINFO, (unsigned long)cr) < 0) {
		ERR("Get connection info failed: %s: errno=%d\n",
			strerror(errno), errno);
		free(cr);
		hci_close_dev(dd);
		usleep(2100000);
		goto cleanup2;
	}

	if (hci_disconnect(dd, htobs(cr->conn_info->handle),
			   HCI_OE_USER_ENDED_CONNECTION, 2000) < 0) {
		ERR("Disconnect failed: %s: errno=%d\n",
			strerror(errno), errno);
		free(cr);
		hci_close_dev(dd);
		usleep(2100000);
		goto cleanup2;
	}

	free(cr);

	hci_close_dev(dd);

      cleanup2:
	close(sk);

      cleanup:
	terminate = 1;		/* Allow BT daemon to exit */
	l2ping_result = err;
	return (void *)err;
}

static void *l2test(void *arg)
{
	int sk;
	struct sockaddr_l2 addr;
	int opt;
	int err;
	struct l2cap_options opts;
	socklen_t optlen;
	char *remote_bdaddr = (char *)arg;
	struct hci_conn_info_req *cr;
	int dd;

	/* Create socket */
	sk = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (sk < 0) {
		ERR("Can't create socket: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup;
	}

	/* Bind to local address */
	memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, BDADDR_ANY);

	if (bind(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		ERR("Can't bind socket: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup2;
	}

	/* Get default options */
	memset(&opts, 0, sizeof(opts));
	optlen = sizeof(opts);

	if (getsockopt(sk, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen) < 0) {
		ERR("Can't get default L2CAP options: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup2;
	}

	/* Set new options */
	opts.omtu = 0;
	opts.imtu = 672;

	if (setsockopt(sk, SOL_L2CAP, L2CAP_OPTIONS, &opts, sizeof(opts)) < 0) {
		ERR("Can't set L2CAP options: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup2;
	}

	opt = L2CAP_LM_AUTH | L2CAP_LM_ENCRYPT;

	if (setsockopt(sk, SOL_L2CAP, L2CAP_LM, &opt, sizeof(opt)) < 0) {
		ERR("Can't set L2CAP link mode: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup2;
	}

	memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	str2ba(remote_bdaddr, &addr.l2_bdaddr);
	addr.l2_psm = htobs(3);

	if (connect(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		ERR("Can't connect socket: %s: errno=%d\n",
			strerror(errno), errno);
		err = errno;
		goto cleanup3;
	}

	err = 0;

	shutdown(sk, SHUT_RDWR);

	/* Wait for L2CAP disconnect response from remote device */
	usleep(100000);

      cleanup3:
	/* Try to explicitly disconnect baseband connection
	 * If encounter failure, then sleep for slightly longer
	 * than 2 seconds to allow baseband connection to timeout
	 * and self-terminate.
	 */
	dd = hci_open_dev(0);
	if (dd < 0) {
		ERR("HCI device open failed: %s: errno=%d\n",
			strerror(errno), errno);
		usleep(2100000);
		goto cleanup2;
	}

	cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
	if (!cr) {
		ERR("Can't allocate memory: %s: errno=%d\n",
			strerror(errno), errno);
		hci_close_dev(dd);
		usleep(2100000);
		goto cleanup2;
	}

	str2ba((char *)arg, &cr->bdaddr);
	cr->type = ACL_LINK;

	if (ioctl(dd, HCIGETCONNINFO, (unsigned long)cr) < 0) {
		ERR("Get connection info failed: %s: errno=%d\n",
			strerror(errno), errno);
		free(cr);
		hci_close_dev(dd);
		usleep(2100000);
		goto cleanup2;
	}

	if (hci_disconnect(dd, htobs(cr->conn_info->handle),
			   HCI_OE_USER_ENDED_CONNECTION, 2000) < 0) {
		ERR("Disconnect failed: %s: errno=%d\n",
			strerror(errno), errno);
		free(cr);
		hci_close_dev(dd);
		usleep(2100000);
		goto cleanup2;
	}

	free(cr);

	hci_close_dev(dd);

      cleanup2:
	close(sk);
      cleanup:
	terminate = 1;		/* Allow BT daemon to exit */
	l2test_result = err;
	return (void *)err;
}

int main(int argc, char *argv[])
{
	int err = 0;
	int fd;

	if (argc < 2)
		return -1;

	if (!strcmp(argv[1], "hci_cmd")) {
		/* USAGE: bthelp hci_cmd timeout <command> */
		int timeout, i, len;
		unsigned char *ptr = hci_buf;
		if (argc < 4) {
			ERR("bthelp usage: bthelp hci_cmd timeout <cmd>\n");
			err = EINVAL;
			goto cleanup;
		}
		// currently ignore timeout
		//hci_cmd_timeout = argv[3];
		for (i = 3, len = 0; i < argc && len < (int) sizeof(hci_buf); i++, len++)
	                *ptr++ = (uint8_t) strtol(argv[i], NULL, 16);

		hci_cmd_len = len;

		err = hcid("0000", 3, NULL);
		if (err) {
			ERR("l2test hci_cmd: err=%d\n", err);
		}
	}
	else if (!strcmp(argv[1], "l2ping")) {

		/* USAGE: bthelp l2ping <remote bdaddr> <pincode> */

		if (argc < 4) {
			ERR
			    ("bthelp usage: bthelp l2ping <remote bdaddr> <pincode>\n");
			err = EINVAL;
			goto cleanup;
		}
#ifndef USE_TI_ST
		fd = hciattach_start(UART_PORT, 3000000, NULL, NULL);
		if (fd < 0) {
			ERR("bthelp: hciattach_start failed: fd=%d\n",
				   fd);
			err = fd;
			goto cleanup;
		}
#endif
		err = hcid(argv[3], 2, argv[2]);

		if (err) {
			ERR("bthelp: l2ping failed: err=%d\n", err);
		}
#ifndef USE_TI_ST
		if (hciattach_stop(fd) < 0) {
			ERR("bthelp: hciattach_stop failed\n");
		}
#endif
	} else if (!strcmp(argv[1], "l2test")) {

		/* USAGE: bthelp l2test <remote bdaddr> <pincode> */

		if (argc < 4) {
			ERR
			    ("bthelp usage: bthelp l2test <remote bdaddr> <pincode>\n");
			err = EINVAL;
			goto cleanup;
		}
#ifndef USE_TI_ST
		fd = hciattach_start(UART_PORT, 3000000, NULL, NULL);
		if (fd < 0) {
			ERR("bthelp: hciattach_start failed: fd=%d\n",
				   fd);
			err = fd;
			goto cleanup;
		}
#endif
		err = hcid(argv[3], 1, argv[2]);

		if (err) {
			ERR("bthelp: l2test failed: err=%d\n", err);
		}
#ifndef USE_TI_ST
		if (hciattach_stop(fd) < 0) {
			ERR("bthelp: hciattach_stop failed\n");
		}
#endif
	} else if (!strcmp(argv[1], "hciattach")) {

		/* USAGE: bthelp hciattach <firmware file> <sleep mode> */

		if (argc < 4) {
			ERR
			    ("bthelp usage: bthelp hciattach <firmware file> <sleep mode>\n");
			ERR("  sleep mode: disable, hcill, 6wire\n");
			err = EINVAL;
			goto cleanup;
		}
#ifdef USE_TI_ST
		err = 0;
#else
		fd = hciattach_start(UART_PORT, 3000000, argv[2], argv[3]);
		if (fd < 0) {
			ERR("bthelp: hciattach failed: fd=%d\n", fd);
			err = fd;
			goto cleanup;
		}
		err = hciattach_stop(fd);
		if (err) {
			ERR("bthelp: hciattach_stop failed: err=%d\n",
				   err);
		}
#endif
	} else if (!strcmp(argv[1], "daemon")) {

		/* USAGE: bthelp daemon <pincode> */
		if (argc < 3) {
			ERR("bthelp usage: bthelp daemon <pincode>\n");
			err = EINVAL;
			goto cleanup;
		}
		err = hcid(argv[2], 0, NULL);
		if (err) {
			ERR("bthelp: hcid daemon failed: err=%d\n", err);
		}
	} else {
		err = EINVAL;
	}

      cleanup:

	DBG("bthelp exit with code %d", err);
	return err;
}
