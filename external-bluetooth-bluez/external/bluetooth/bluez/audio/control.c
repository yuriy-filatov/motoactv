/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2010  Nokia Corporation
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
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

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/l2cap.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <gdbus.h>

#include "log.h"
#include "error.h"
#include "uinput.h"
#include "adapter.h"
#include "../src/device.h"
#include "device.h"
#include "manager.h"
#include "avdtp.h"
#include "control.h"
#include "sdpd.h"
#include "glib-helper.h"
#include "btio.h"
#include "dbus-common.h"

#define AVCTP_PSM 23

/* Message types */
#define AVCTP_COMMAND		0
#define AVCTP_RESPONSE		1

/* Packet types */
#define AVCTP_PACKET_SINGLE	0
#define AVCTP_PACKET_START	1
#define AVCTP_PACKET_CONTINUE	2
#define AVCTP_PACKET_END	3

/* ctype entries */
#define CTYPE_CONTROL		0x0
#define CTYPE_STATUS		0x1
#define CTYPE_NOT_IMPLEMENTED	0x8
#define CTYPE_ACCEPTED		0x9
#define CTYPE_REJECTED		0xA
#define CTYPE_STABLE		0xC
#define CTYPE_NOTIFY		0x3
#define CTYPE_INTERIM		0xF
#define CTYPE_CHANGED		0xD

/* opcodes */
#define OP_UNITINFO		0x30
#define OP_SUBUNITINFO		0x31
#define OP_PASSTHROUGH		0x7c
#define OP_VENDOR_DEP		0x0

/* subunits of interest */
#define SUBUNIT_PANEL		0x09

/* AVRCP Packet Types */
#define AVRCP_PACKET_SINGLE		0
#define AVRCP_PACKET_START		1
#define AVRCP_PACKET_CONTINUE		2
#define AVRCP_PACKET_END		3

/* operands in passthrough commands */
#define VOL_UP_OP		0x41
#define VOL_DOWN_OP		0x42
#define MUTE_OP			0x43
#define PLAY_OP			0x44
#define STOP_OP			0x45
#define PAUSE_OP		0x46
#define RECORD_OP		0x47
#define REWIND_OP		0x48
#define FAST_FORWARD_OP		0x49
#define EJECT_OP		0x4a
#define FORWARD_OP		0x4b
#define BACKWARD_OP		0x4c
#define POWER_SONG_OP   0x71

#define QUIRK_NO_RELEASE	1 << 0
/*AVRCP 1.3  Events*/
#define EVENT_PLAYBACK_STATUS_CHANGED	0x1
#define EVENT_TRACK_CHANGED		0x2
#define EVENT_VOLUME_CHANGED		0xd

/*AVRCP 1.3 Capability IDs*/
#define CAPABILITY_COMPANY_ID		0X2
#define CAPABILITY_EVENTS_SUPPORTED	0X3

/*AVRCP 1.3 PDU IDS */
#define PDU_GET_CAPABILITY		0X10
#define PDU_GET_ELEMENT_ATTRIBUTES	0X20
#define PDU_GET_PLAY_STATUS		0X30
#define PDU_REGISTER_NOTIFICATION	0X31
#define PDU_REQ_CONTINUE_RESPONSE	0x40
#define PDU_ABORT_CONTINUE_RESPONSE	0x41
#define PDU_SET_ABSOLUTE_VOLUME		0x50

/*AVRCP 1.3 PLAYBACK STATUS */
#define STATUS_STOPPED			0X00
#define STATUS_PLAYING			0X01
#define STATUS_PAUSED			0X02
#define STATUS_FWD_SEEK			0X03
#define STATUS_REV_SEEK			0X04
#define STATUS_ERROR			0XFF

/*AVRCP 1.3 Metadata Errors*/
#define ERROR_PDU_COMMAND		0x00
#define ERROR_PDU_PARAMETER		0x01
#define ERROR_PARAMETER_CORRUPTED	0x02
#define ERROR_INTERNAL			0X03

#define AVRCP_MAX_PKT_SIZE		512

#define META_DATA_TITLE			(1<<0)
#define META_DATA_ARTIST			(1<<1)
#define META_DATA_ALBUM			(1<<2)
#define META_DATA_NUMBER_OF_MEDIA		(1<<3)
#define META_DATA_TOTAL_NUMBER_OF_MEDIA	(1<<4)
#define META_DATA_GENRE			(1<<5)
#define META_DATA_PLAYING_TIME			(1<<6)

/* AVRCP 1.3 Metadata default strings values */
#define DEFAULT_DURATION		"12345"
#define DEFAULT_DURATION_LEN            10
#define DEFAULT_TRACK_NO		"1"
#define DEFAULT_TRACK_NO_LEN            10
#define DEFAULT_NO_OF_TRACKS		"1"
#define DEFAULT_NO_OF_TRACKS_LEN        10

#define DEFAULT_META_DATA_COUNT		6
#define DEFAULT_META_DATA_MASK		127

#define START_ATTID		0x01
#define END_ATTID		0x07

#define AVRCP_TG_UUID	"0000110C-0000-1000-8000-00805F9B34FB"
#define AVRCP_CAT2_SUPPORT	(1<<1)
#define ABSOLUTE_VOLUME_PROFILE_VERSION	0x0104
#define DEFAULT_VOLUME_LEVEL	5

#define DBG(fmt, arg...)  printf("DEBUG: %s: " fmt "\n" , __FUNCTION__ , ## arg)

/* As per AVRCP 1.4 Spec, the absolute volume is 7bit length data.
 * ABSOLUTE_VOLUME is a macro to read the 7bit data from a byte.
 */
#define ABSOLUTE_VOLUME(x)	(x & (~(1<<7)))
/* Absolute volume range is 0 to 0x7F. In our phone, there are 16 volume
 * levels are available. so the scale will be 16*7.937 = 0x7F;
 */
#define ABSOLUTE_VOLUME_SCALE	7.937


/* PTS TC default values */
#define PTS_TITLE "This is Testing Unknown Title. This is Testing Unknown Tit \
		le.This is Testing Unknown title.This is Testing Unknown Title.This is \
		Testing Unknown Title.This is Testing."
#define MAX_TITLE_LENGTH 205

#define PTS_ARTIST  "This is Testing Unknown Artist. This is Testing Unknown \
		Artist.This is Testing Unknown Artist.This is Testing Unknown Artist. \
		This is Testing Unknown Artist.This is Testing."
#define MAX_ARTIST_LENGTH 210

#define PTS_ALBUM "This is Testing Unknown Album. This is Testing Unknown \
		Album.This is Testing Unknown Album.This is Testing Unknown Album. \
		This is Testing Unknown Album.This is Testing."
#define MAX_ALBUM_LENGTH 205

static uint8_t transaction_playback_event = 0;
static uint8_t transaction_track_event = 0;

static DBusConnection *connection = NULL;
static gchar *input_device_name = NULL;
int metadata_support , controller_support;
static GSList *servers = NULL;

#if __BYTE_ORDER == __LITTLE_ENDIAN

struct avctp_header {
	uint8_t ipid:1;
	uint8_t cr:1;
	uint8_t packet_type:2;
	uint8_t transaction:4;
	uint16_t pid;
} __attribute__ ((packed));
#define AVCTP_HEADER_LENGTH 3

struct avrcp_header {
	uint8_t code:4;
	uint8_t _hdr0:4;
	uint8_t subunit_id:3;
	uint8_t subunit_type:5;
	uint8_t opcode;
} __attribute__ ((packed));
#define AVRCP_HEADER_LENGTH 3

struct meta_data_params {
	uint32_t cid:24;
	uint32_t pduid:8;
	uint8_t  pktType:2;
	uint8_t  reserved:6;
	uint16_t paramLen;
	uint8_t  capId;
} __attribute__ ((packed));
#define META_DATA_PARAMS_LENGTH 8

struct media_attr {
	uint32_t att_id;
	uint16_t char_set_id;
	uint16_t att_val_len;
	unsigned char att_val[1];
} __attribute__ ((packed));
#define MEDIA_ATTR_LEN 8

#elif __BYTE_ORDER == __BIG_ENDIAN
struct avctp_header {
	uint8_t transaction:4;
	uint8_t packet_type:2;
	uint8_t cr:1;
	uint8_t ipid:1;
	uint16_t pid;
} __attribute__ ((packed));
#define AVCTP_HEADER_LENGTH 3

struct avrcp_header {
	uint8_t _hdr0:4;
	uint8_t code:4;
	uint8_t subunit_type:5;
	uint8_t subunit_id:3;
	uint8_t opcode;
} __attribute__ ((packed));
#define AVRCP_HEADER_LENGTH 3

struct meta_data_params {
	uint32_t pduid:8;
	uint32_t cid:24;
	uint8_t  reserved:6;
	uint8_t  pktType:2;
	uint16_t paramLen;
	uint8_t  capId;
} __attribute__ ((packed));
#define META_DATA_PARAMS_LENGTH 8

struct media_attr {
	uint32_t att_id;
	uint16_t char_set_id;
	uint16_t att_val_len;
	unsigned char att_val[1];
} __attribute__ ((packed));
#define MEDIA_ATTR_LEN 8
#else
#error "Unknown byte order"
#endif

struct avctp_state_callback {
	avctp_state_cb cb;
	void *user_data;
	unsigned int id;
};

struct avctp_server {
	bdaddr_t src;
	GIOChannel *io;
	uint32_t tg_record_id;
	uint32_t ct_record_id;
};

struct control {
	struct audio_device *dev;

	avctp_state_t state;

	int uinput;

	GIOChannel *io;
	guint io_id;

	uint16_t mtu;

	gboolean target;
	uint8_t key_quirks[256];
	gboolean target_cat2_support;

	gboolean ignore_pause;
};

static struct {
	const char *name;
	uint8_t avrcp;
	uint16_t uinput;
} key_map[] = {
	{ "PLAY",		PLAY_OP,		KEY_PLAYCD },
	{ "STOP",		STOP_OP,		KEY_STOPCD },
	{ "PAUSE",		PAUSE_OP,		KEY_PAUSECD },
	{ "FORWARD",		FORWARD_OP,		KEY_NEXTSONG },
	{ "BACKWARD",		BACKWARD_OP,		KEY_PREVIOUSSONG },
	{ "REWIND",		REWIND_OP,		KEY_REWIND },
	{ "FAST FORWARD",	FAST_FORWARD_OP,	KEY_FASTFORWARD },
	{ "POWER SONG",	POWER_SONG_OP,	KEY_POWER_SONG },
	{ NULL }
};

static char* title = NULL;
static char* artist = NULL;
static char* album = NULL;
static char* genre = "Unknown Genere";
static char* no_of_tracks = NULL;
static char* track_no = NULL;
static char* duration = NULL;
static char* remaning_meta_data = NULL;
static int remaning_meta_data_len = 0;
static uint32_t current_status = STATUS_PAUSED;


static int send_play_status(struct control *control,
			uint32_t duration, uint32_t position, uint32_t status);
int send_meta_data(struct control *control,int transaction_label,
			uint8_t att_count,uint8_t meta_data_mask);
static int send_set_absolute_volume(struct control *control, uint8_t level);
static int send_register_volume_changed_event(struct control *control);
static int absolute_volume(int level);


gboolean event_track_changed = FALSE;
gboolean event_playback_status = FALSE;

static uint16_t play_status_transaction_label = 0;

uint16_t transaction_label = 0;

static GSList *avctp_callbacks = NULL;

static void auth_cb(DBusError *derr, void *user_data);

static sdp_record_t *avrcp_ct_record()
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, avctp, avrct, avrct_controller;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t *record;
	sdp_data_t *psm, *version, *features;
	uint16_t lp = AVCTP_PSM, avctp_ver = 0x0103;
	uint16_t avrcp_ver = 0x0104, feat = 0x0002;

	record = sdp_record_alloc();
	if (!record)
		return NULL;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(record, root);

	/* Service Class ID List */
	sdp_uuid16_create(&avrct, AV_REMOTE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &avrct);
	sdp_uuid16_create(&avrct_controller, AV_REMOTE_CONTROLLER_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &avrct_controller);
	sdp_set_service_classes(record, svclass_id);

	/* Protocol Descriptor List */
	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&avctp, AVCTP_UUID);
	proto[1] = sdp_list_append(0, &avctp);
	version = sdp_data_alloc(SDP_UINT16, &avctp_ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(record, aproto);

	/* Bluetooth Profile Descriptor List */
	sdp_uuid16_create(&profile[0].uuid, AV_REMOTE_PROFILE_ID);
	profile[0].version = avrcp_ver;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(record, pfseq);

	features = sdp_data_alloc(SDP_UINT16, &feat);
	sdp_attr_add(record, SDP_ATTR_SUPPORTED_FEATURES, features);

	sdp_set_info_attr(record, "AVRCP CT", 0, 0);

	free(psm);
	free(version);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(pfseq, 0);
	sdp_list_free(aproto, 0);
	sdp_list_free(root, 0);
	sdp_list_free(svclass_id, 0);

	return record;
}

static sdp_record_t *avrcp_tg_record()
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, avctp, avrtg;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t *record;
	sdp_data_t *psm, *version, *features;
	uint16_t lp, avctp_ver, avrcp_ver, feat = 0x000f;

	lp = AVCTP_PSM;
	avctp_ver = 0x0103;
	if (metadata_support) {
		avrcp_ver = 0x0103;
	} else {
		avrcp_ver = 0x0100;
	}

#ifdef ANDROID
	feat = 0x0001;
#endif
	record = sdp_record_alloc();
	if (!record)
		return NULL;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(record, root);

	/* Service Class ID List */
	sdp_uuid16_create(&avrtg, AV_REMOTE_TARGET_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &avrtg);
	sdp_set_service_classes(record, svclass_id);

	/* Protocol Descriptor List */
	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&avctp, AVCTP_UUID);
	proto[1] = sdp_list_append(0, &avctp);
	version = sdp_data_alloc(SDP_UINT16, &avctp_ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(record, aproto);

	/* Bluetooth Profile Descriptor List */
	sdp_uuid16_create(&profile[0].uuid, AV_REMOTE_PROFILE_ID);
	profile[0].version = avrcp_ver;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(record, pfseq);

	features = sdp_data_alloc(SDP_UINT16, &feat);
	sdp_attr_add(record, SDP_ATTR_SUPPORTED_FEATURES, features);

	sdp_set_info_attr(record, "AVRCP TG", 0, 0);

	free(psm);
	free(version);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);
	sdp_list_free(pfseq, 0);
	sdp_list_free(root, 0);
	sdp_list_free(svclass_id, 0);

	return record;
}

static int send_event(int fd, uint16_t type, uint16_t code, int32_t value)
{
	struct uinput_event event;

	memset(&event, 0, sizeof(event));
	event.type	= type;
	event.code	= code;
	event.value	= value;

	return write(fd, &event, sizeof(event));
}

static void send_key(int fd, uint16_t key, int pressed)
{
	if (fd < 0)
		return;

	send_event(fd, EV_KEY, key, pressed);
	send_event(fd, EV_SYN, SYN_REPORT, 0);
}

static void handle_panel_passthrough(struct control *control,
					const unsigned char *operands,
					int operand_count)
{
	const char *status;
	int pressed, i;

	if (operand_count == 0)
		return;

	if (operands[0] & 0x80) {
		status = "released";
		pressed = 0;
	} else {
		status = "pressed";
		pressed = 1;
	}

//#ifdef ANDROID
#if 0
	if ((operands[0] & 0x7F) == PAUSE_OP) {
		if (!sink_is_streaming(control->dev)) {
			if (pressed) {
				uint8_t key_quirks =
					control->key_quirks[PAUSE_OP];
				DBG("AVRCP: Ignoring Pause key - pressed");
				if (!(key_quirks & QUIRK_NO_RELEASE))
					control->ignore_pause = TRUE;
				return;
			} else if (!pressed && control->ignore_pause) {
				DBG("AVRCP: Ignoring Pause key - released");
				control->ignore_pause = FALSE;
				return;
			}
		}
	}
#endif

	for (i = 0; key_map[i].name != NULL; i++) {
		uint8_t key_quirks;

		if ((operands[0] & 0x7F) != key_map[i].avrcp)
			continue;

		DBG("AVRCP: %s %s", key_map[i].name, status);

		key_quirks = control->key_quirks[key_map[i].avrcp];

		if (key_quirks & QUIRK_NO_RELEASE) {
			if (!pressed) {
				DBG("AVRCP: Ignoring release");
				break;
			}

			DBG("AVRCP: treating key press as press + release");
			send_key(control->uinput, key_map[i].uinput, 1);
			send_key(control->uinput, key_map[i].uinput, 0);
			break;
		}

		send_key(control->uinput, key_map[i].uinput, pressed);
		break;
	}

	if (key_map[i].name == NULL)
		DBG("AVRCP: unknown button 0x%02X %s",
						operands[0] & 0x7F, status);
}

static void avctp_disconnected(struct audio_device *dev)
{
	struct control *control = dev->control;

	if (!control)
		return;

	if (control->io) {
		g_io_channel_shutdown(control->io, TRUE, NULL);
		g_io_channel_unref(control->io);
		control->io = NULL;
	}

	if (control->io_id) {
		g_source_remove(control->io_id);
		control->io_id = 0;

		if (control->state == AVCTP_STATE_CONNECTING)
			audio_device_cancel_authorization(dev, auth_cb,
								control);
	}

	if (control->uinput >= 0) {
		char address[18];

		ba2str(&dev->dst, address);
		DBG("AVRCP: closing uinput for %s", address);

		ioctl(control->uinput, UI_DEV_DESTROY);
		close(control->uinput);
		control->uinput = -1;
	}
}

static void avctp_set_state(struct control *control, avctp_state_t new_state)
{
	GSList *l;
	struct audio_device *dev = control->dev;
	avdtp_session_state_t old_state = control->state;
	gboolean value;

	switch (new_state) {
	case AVCTP_STATE_DISCONNECTED:
		DBG("AVCTP Disconnected");
		if (metadata_support) {
			event_track_changed = FALSE;
			event_playback_status = FALSE;
		}
		avctp_disconnected(control->dev);

		if (old_state != AVCTP_STATE_CONNECTED)
			break;

		value = FALSE;
		g_dbus_emit_signal(dev->conn, dev->path,
					AUDIO_CONTROL_INTERFACE,
					"Disconnected", DBUS_TYPE_INVALID);
		emit_property_changed(dev->conn, dev->path,
					AUDIO_CONTROL_INTERFACE, "Connected",
					DBUS_TYPE_BOOLEAN, &value);

		if (!audio_device_is_active(dev, NULL))
			audio_device_set_authorized(dev, FALSE);

		break;
	case AVCTP_STATE_CONNECTING:
		DBG("AVCTP Connecting");
		break;
	case AVCTP_STATE_CONNECTED:
		DBG("AVCTP Connected");
		value = TRUE;
		g_dbus_emit_signal(control->dev->conn, control->dev->path,
				AUDIO_CONTROL_INTERFACE, "Connected",
				DBUS_TYPE_INVALID);
		emit_property_changed(control->dev->conn, control->dev->path,
				AUDIO_CONTROL_INTERFACE, "Connected",
				DBUS_TYPE_BOOLEAN, &value);
		break;
	default:
		error("Invalid AVCTP state %d", new_state);
		return;
	}

	control->state = new_state;

	for (l = avctp_callbacks; l != NULL; l = l->next) {
		struct avctp_state_callback *cb = l->data;
		cb->cb(control->dev, old_state, new_state, cb->user_data);
	}
}

static gboolean control_cb(GIOChannel *chan, GIOCondition cond,
				gpointer data)
{
	struct control *control = data;
	unsigned char buf[1024], *operands;
	struct avctp_header *avctp;
	struct avrcp_header *avrcp;
	int ret, packet_size, operand_count, sock;
	unsigned char *tOp = NULL;
	struct meta_data_params *mOperands = NULL;
	struct media_attr *mAttributes;
	int loop = 0;
	uint8_t att_count = 0;
	uint32_t *identifier;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		goto failed;

	sock = g_io_channel_unix_get_fd(control->io);

	ret = read(sock, buf, sizeof(buf));
	if (ret <= 0)
		goto failed;

	DBG("Got %d bytes of data for AVCTP session %p", ret, control);

	if ((unsigned int) ret < sizeof(struct avctp_header)) {
		error("Too small AVCTP packet");
		goto failed;
	}

	packet_size = ret;

	avctp = (struct avctp_header *) buf;

	DBG("AVCTP transaction %u, packet type %u, C/R %u, IPID %u, "
			"PID 0x%04X",
			avctp->transaction, avctp->packet_type,
			avctp->cr, avctp->ipid, ntohs(avctp->pid));

	ret -= sizeof(struct avctp_header);
	if ((unsigned int) ret < sizeof(struct avrcp_header)) {
		error("Too small AVRCP packet");
		goto failed;
	}

	avrcp = (struct avrcp_header *) (buf + sizeof(struct avctp_header));

	ret -= sizeof(struct avrcp_header);

	operands = buf + sizeof(struct avctp_header) +
			sizeof(struct avrcp_header);
	if (metadata_support) {
		mOperands = (struct meta_data_params *) (buf +
				sizeof(struct avctp_header) +
				sizeof(struct avrcp_header));
	}
	operand_count = ret;

	DBG("AVRCP %s 0x%01X, subunit_type 0x%02X, subunit_id 0x%01X, "
			"opcode 0x%02X, %d operands",
			avctp->cr ? "response" : "command",
			avrcp->code, avrcp->subunit_type, avrcp->subunit_id,
			avrcp->opcode, operand_count);
	if (controller_support) {
		if (avctp->cr == AVCTP_RESPONSE) {
			if (((avrcp->code == CTYPE_CHANGED) ||
				(avrcp->code == CTYPE_INTERIM) ||
				(avrcp->code == CTYPE_ACCEPTED)) &&
				avrcp->opcode == OP_VENDOR_DEP &&
				avrcp->subunit_type == SUBUNIT_PANEL) {
				if (mOperands->pduid == PDU_REGISTER_NOTIFICATION) {
					if (mOperands->capId == EVENT_VOLUME_CHANGED) {
						DBG ("Received volume_changed notification");
						uint8_t *operands = NULL;
						uint16_t volume = 0;
						dbus_uint16_t level = 0;
						operands = (uint8_t *)mOperands;
						operands+= META_DATA_PARAMS_LENGTH;
						DBG ("The new absolute volume is %d", *operands);
						volume= ABSOLUTE_VOLUME(*operands);
						DBG ("Absolute volume is %d", volume);
						level = absolute_volume(volume);
						DBG ("Obtained volume level is %d", level);
						g_dbus_emit_signal(control->dev->conn,
							control->dev->path,
							AUDIO_CONTROL_INTERFACE,
							"AbsoluteVolumeChanged",
							DBUS_TYPE_UINT16,
							&level,
							DBUS_TYPE_INVALID);
						DBG ("AbsoluteVolumeChanged Signal Sent");
						if (avrcp->code == CTYPE_CHANGED)
							send_register_volume_changed_event(control);
					}
				} else if (mOperands->pduid == PDU_SET_ABSOLUTE_VOLUME) {
					uint16_t absolute_vol = 0;
					dbus_uint16_t volume_level = 0;
					DBG ("Received absolute volume %d", mOperands->capId);
					absolute_vol = ABSOLUTE_VOLUME(mOperands->capId);
					DBG ("Absolute volume is %d", absolute_vol);
					volume_level = absolute_volume(absolute_vol);
					DBG ("Obtained volume level is %d", volume_level);
					g_dbus_emit_signal(control->dev->conn,
							control->dev->path,
							AUDIO_CONTROL_INTERFACE,
							"AbsoluteVolumeChanged",
							DBUS_TYPE_UINT16,
							&volume_level,
							DBUS_TYPE_INVALID);
					DBG ("AbsoluteVolumeChanged Signal Sent");
				}
			}
			info("Received response pkt from Target");
			return TRUE;
		}
	}
	if (avctp->packet_type != AVCTP_PACKET_SINGLE) {
		avctp->cr = AVCTP_RESPONSE;
		avrcp->code = CTYPE_NOT_IMPLEMENTED;
	} else if (avctp->pid != htons(AV_REMOTE_SVCLASS_ID)) {
		avctp->ipid = 1;
		avctp->cr = AVCTP_RESPONSE;
		avrcp->code = CTYPE_REJECTED;
	} else if (avctp->cr == AVCTP_COMMAND &&
			avrcp->code == CTYPE_CONTROL &&
			avrcp->subunit_type == SUBUNIT_PANEL &&
			avrcp->opcode == OP_PASSTHROUGH) {
		uint8_t *tmp = (uint8_t *)avrcp;
		tmp+= 3;
		DBG ("Handling passthr tmp = %d",*tmp);
		if(*tmp == 0x7E || *tmp == 0xFE) {
			avctp->cr = AVCTP_RESPONSE;
			avrcp->code = CTYPE_REJECTED;
		} else {
			handle_panel_passthrough(control, operands, operand_count);
			avctp->cr = AVCTP_RESPONSE;
			avrcp->code = CTYPE_ACCEPTED;
		}
	} else if (avctp->cr == AVCTP_COMMAND &&
			avrcp->code == CTYPE_STATUS &&
			(avrcp->opcode == OP_UNITINFO
			|| avrcp->opcode == OP_SUBUNITINFO)) {
		avctp->cr = AVCTP_RESPONSE;
		avrcp->code = CTYPE_STABLE;
		/* The first operand should be 0x07 for the UNITINFO response.
		 * Neither AVRCP (section 22.1, page 117) nor AVC Digital
		 * Interface Command Set (section 9.2.1, page 45) specs
		 * explain this value but both use it */
		if (operand_count >= 1 && avrcp->opcode == OP_UNITINFO)
			operands[0] = 0x07;
		if (operand_count >= 2)
			operands[1] = SUBUNIT_PANEL << 3;
		DBG("reply to %s", avrcp->opcode == OP_UNITINFO ?
				"OP_UNITINFO" : "OP_SUBUNITINFO");
	} else if (metadata_support) {
		if (avctp->cr == AVCTP_COMMAND &&
			(avrcp->code == CTYPE_STATUS ||
			avrcp->code == CTYPE_CONTROL ||
			avrcp->code == CTYPE_NOTIFY) &&
			avrcp->opcode == OP_VENDOR_DEP &&
			avrcp->subunit_type == SUBUNIT_PANEL) {

			//Filling the company id for Response Motorola = 0x08
			tOp = (char *)(buf + sizeof(struct avctp_header) +
			sizeof(struct avrcp_header));
			tOp++;
			tOp++;

			if (mOperands->pduid == PDU_GET_CAPABILITY) {
				avctp->cr = AVCTP_RESPONSE;
				avrcp->code = CTYPE_STABLE;
				mOperands->reserved = 0x00;
				mOperands->pktType = 0x0;
				if (mOperands->capId == CAPABILITY_COMPANY_ID) {
					tOp = (char *)(buf +
					sizeof(struct avctp_header) +
					sizeof(struct avrcp_header) +
					sizeof(struct meta_data_params));
					//Capability Count
					*tOp = 0x1;
					tOp++;
					//Company Id
					*tOp = 0x0;
					tOp++;
					*tOp = 0x0;
					tOp++;
					*tOp = 0x8;
					mOperands->paramLen = htons(0x5);
					packet_size = packet_size + 4;
				} else if (mOperands->capId ==
					CAPABILITY_EVENTS_SUPPORTED) {
					tOp = (char *)(buf +
					sizeof(struct avctp_header) +
					sizeof(struct avrcp_header) +
					sizeof(struct meta_data_params));
					//Capability Count
					*tOp = 0x2;
					mOperands->paramLen = htons(0x4);
					tOp++;

					*tOp = EVENT_PLAYBACK_STATUS_CHANGED;
					tOp++;
					*tOp = EVENT_TRACK_CHANGED;
					packet_size = packet_size + 3;
				} else {
					avctp->cr = AVCTP_RESPONSE;
					avrcp->code = CTYPE_REJECTED;
					mOperands->paramLen = htons(0x1);
					mOperands->capId = ERROR_PDU_PARAMETER;
				}
			//Get Element Attributes command
			} else if (mOperands->pduid == PDU_GET_ELEMENT_ATTRIBUTES) {
				uint8_t meta_data_mask = 0;
				operands = (uint8_t *)mOperands;
				operands+= 2 * META_DATA_PARAMS_LENGTH - 1;
				att_count = *operands;
				DBG ("Att_count = %d",att_count);
				if (att_count == 0)	{
					meta_data_mask = DEFAULT_META_DATA_MASK;
					DBG ("Meta_data_mask is %d", meta_data_mask);
					att_count = DEFAULT_META_DATA_COUNT;
					send_meta_data(control, avctp->transaction,
							att_count, meta_data_mask);
					return TRUE;
				} else {
					operands++;
					identifier = (uint32_t *)operands;
					uint32_t att_id = 0;
					uint8_t tmp_att_count = att_count;
					while (tmp_att_count > 0) {
						if(identifier != NULL) {
							att_id = htonl((*identifier));
							DBG ("Att_id is = 	%d", att_id);
							if (att_id >= START_ATTID && att_id <= END_ATTID) {
								meta_data_mask|= 1 << (att_id - 1);
							}
						}
						tmp_att_count--;
						identifier++;
					}
					DBG ("Meta_data_mask is %d", meta_data_mask);
					if ((att_count == 1) &&
							((meta_data_mask & META_DATA_GENRE) != 0x0)) {
						DBG ("Meta_data_genre is not supported");
						avctp->cr = AVCTP_RESPONSE;
						avrcp->code = CTYPE_REJECTED;
						mOperands->paramLen = htons(0x1);
						mOperands->capId = ERROR_PDU_PARAMETER;
					} else {
						send_meta_data(control, avctp->transaction,
								att_count, meta_data_mask);
						return TRUE;
					}
				}
			} else if (mOperands->pduid == PDU_REQ_CONTINUE_RESPONSE) {
					DBG ("PDU_REQ_CONTINUE_RESPONSE received");
					send_meta_data_continue_response(control, avctp->transaction);
					return TRUE;
				} else if (mOperands->pduid == PDU_ABORT_CONTINUE_RESPONSE) {
					DBG ("Abort continue response");
					avctp->cr = AVCTP_RESPONSE;
					avrcp->code = CTYPE_STABLE;
					mOperands->paramLen = 0;
					if (remaning_meta_data != NULL)
						free(remaning_meta_data);

					remaning_meta_data_len = 0;
					packet_size = packet_size - 1;
			} else if (mOperands->pduid == PDU_REGISTER_NOTIFICATION) {
				avctp->cr = AVCTP_RESPONSE;
				avrcp->code = CTYPE_INTERIM;
				mOperands->reserved = 0x00;
				mOperands->pktType = 0x0;
				mOperands->paramLen = htons(0x9);
				if (mOperands->capId == EVENT_PLAYBACK_STATUS_CHANGED) {
					event_playback_status = TRUE;
					tOp = (char *)(buf +
						sizeof(struct avctp_header) +
						sizeof(struct avrcp_header) +
						sizeof(struct meta_data_params));
					*tOp = current_status;
					mOperands->paramLen = htons(0x2);
					packet_size = packet_size - 3;
					transaction_playback_event = avctp->transaction;

				} else if (mOperands->capId == EVENT_TRACK_CHANGED) {
					event_track_changed = TRUE;
					tOp = (char *)(buf + sizeof(struct avctp_header)
						+ sizeof(struct avrcp_header)
						+ sizeof(struct meta_data_params));
					for (loop = 0;loop<8;loop++,tOp++)
						*tOp = 0xFF;
					mOperands->paramLen = htons(0x9);

					packet_size = packet_size + 4;
					transaction_track_event = avctp->transaction;

				} else {
					avctp->cr = AVCTP_RESPONSE;
					avrcp->code = CTYPE_REJECTED;
					mOperands->paramLen = htons(0x1);
					mOperands->capId = ERROR_PDU_PARAMETER;
				}
			} else if (mOperands->pduid == PDU_GET_PLAY_STATUS) {
				play_status_transaction_label = avctp->transaction;
				if (!(strcmp(track_no, DEFAULT_TRACK_NO))
							&& !(strcmp(no_of_tracks, DEFAULT_NO_OF_TRACKS))
							&& !(strcmp(duration, DEFAULT_DURATION))) {
					send_play_status(control, 0, 0, 0);
					return TRUE;
				} else {
					g_dbus_emit_signal(control->dev->conn,
									control->dev->path,
									AUDIO_CONTROL_INTERFACE,
									"PlayStatusRequested", DBUS_TYPE_INVALID);
					return TRUE;
				}
			} else {
				avctp->cr = AVCTP_RESPONSE;
				avrcp->code = CTYPE_REJECTED;
				if (metadata_support) {
					mOperands->paramLen = htons(0x1);
				}
				mOperands->capId = ERROR_PDU_COMMAND;
				packet_size = packet_size + 1;
			}
		} else {
			avctp->cr = AVCTP_RESPONSE;
			avrcp->code = CTYPE_REJECTED;
			mOperands->paramLen = htons(0);
		}
	} else {
		avctp->cr = AVCTP_RESPONSE;
		avrcp->code = CTYPE_REJECTED;
	}
	ret = write(sock, buf, packet_size);

	return TRUE;

failed:
	DBG("AVCTP session %p got disconnected", control);
	avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
	return FALSE;
}

static int uinput_create(char *name)
{
	struct uinput_dev dev;
	int fd, err, i;

	fd = open("/dev/uinput", O_RDWR);
	if (fd < 0) {
		fd = open("/dev/input/uinput", O_RDWR);
		if (fd < 0) {
			fd = open("/dev/misc/uinput", O_RDWR);
			if (fd < 0) {
				err = errno;
				error("Can't open input device: %s (%d)",
							strerror(err), err);
				return -err;
			}
		}
	}

	memset(&dev, 0, sizeof(dev));
	if (name)
		strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE - 1);

	dev.id.bustype = BUS_BLUETOOTH;
	dev.id.vendor  = 0x0000;
	dev.id.product = 0x0000;
	dev.id.version = 0x0000;

	if (write(fd, &dev, sizeof(dev)) < 0) {
		err = errno;
		error("Can't write device information: %s (%d)",
						strerror(err), err);
		close(fd);
		errno = err;
		return -err;
	}

	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_EVBIT, EV_REP);
	ioctl(fd, UI_SET_EVBIT, EV_SYN);

	for (i = 0; key_map[i].name != NULL; i++)
		ioctl(fd, UI_SET_KEYBIT, key_map[i].uinput);

	if (ioctl(fd, UI_DEV_CREATE, NULL) < 0) {
		err = errno;
		error("Can't create uinput device: %s (%d)",
						strerror(err), err);
		close(fd);
		errno = err;
		return -err;
	}

	return fd;
}

static void init_uinput(struct control *control)
{
	struct audio_device *dev = control->dev;
	char address[18], name[248 + 1], *uinput_dev_name;

	device_get_name(dev->btd_dev, name, sizeof(name));
	if (g_str_equal(name, "Nokia CK-20W")) {
		control->key_quirks[FORWARD_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[BACKWARD_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[PLAY_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[PAUSE_OP] |= QUIRK_NO_RELEASE;
	}
	control->ignore_pause = FALSE;

	ba2str(&dev->dst, address);

	/* Use device name from config file if specified */
	uinput_dev_name = input_device_name;
	if (!uinput_dev_name)
		uinput_dev_name = address;

	control->uinput = uinput_create(uinput_dev_name);
	if (control->uinput < 0)
		error("AVRCP: failed to init uinput for %s", address);
	else
		DBG("AVRCP: uinput initialized for %s", address);
}

static void avctp_connect_cb(GIOChannel *chan, GError *err, gpointer data)
{
	struct control *control = data;
	char address[18];
	uint16_t imtu;
	GError *gerr = NULL;
	int remote_profile_version = 0, remote_supported_feature = 0;

	if (err) {
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
		error("%s", err->message);
		return;
	}

	bt_io_get(chan, BT_IO_L2CAP, &gerr,
			BT_IO_OPT_DEST, &address,
			BT_IO_OPT_IMTU, &imtu,
			BT_IO_OPT_INVALID);
	if (gerr) {
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
		error("%s", gerr->message);
		g_error_free(gerr);
		return;
	}

	DBG("AVCTP: connected to %s", address);

	if (!control->io)
		control->io = g_io_channel_ref(chan);

	init_uinput(control);

	avctp_set_state(control, AVCTP_STATE_CONNECTED);
	control->mtu = imtu;
	control->io_id = g_io_add_watch(chan,
				G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
				(GIOFunc) control_cb, control);

	remote_profile_version = read_profile_version(control->dev->btd_dev,
								AVRCP_TG_UUID);
	DBG ("Remote profile version is %d", remote_profile_version);
	remote_supported_feature = read_supported_feature_value(
								control->dev->btd_dev,
								AVRCP_TG_UUID,
								SDP_ATTR_SUPPORTED_FEATURES);

	if ((remote_profile_version >= ABSOLUTE_VOLUME_PROFILE_VERSION) &&
			(remote_supported_feature & AVRCP_CAT2_SUPPORT)) {
		DBG ("Remote device supports Absolute volume control");
		control->target_cat2_support = TRUE;
	}
}


static void auth_cb(DBusError *derr, void *user_data)
{
	struct control *control = user_data;
	GError *err = NULL;

	if (control->io_id) {
		g_source_remove(control->io_id);
		control->io_id = 0;
	}

	if (derr && dbus_error_is_set(derr)) {
		error("Access denied: %s", derr->message);
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
		return;
	}

	if (!bt_io_accept(control->io, avctp_connect_cb, control,
								NULL, &err)) {
		error("bt_io_accept: %s", err->message);
		g_error_free(err);
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
	}
}

static void avctp_confirm_cb(GIOChannel *chan, gpointer data)
{
	struct control *control = NULL;
	struct audio_device *dev;
	char address[18];
	bdaddr_t src, dst;
	GError *err = NULL;

	bt_io_get(chan, BT_IO_L2CAP, &err,
			BT_IO_OPT_SOURCE_BDADDR, &src,
			BT_IO_OPT_DEST_BDADDR, &dst,
			BT_IO_OPT_DEST, address,
			BT_IO_OPT_INVALID);
	if (err) {
		error("%s", err->message);
		g_error_free(err);
		g_io_channel_shutdown(chan, TRUE, NULL);
		return;
	}

	dev = manager_get_device(&src, &dst, TRUE);
	if (!dev) {
		error("Unable to get audio device object for %s", address);
		goto drop;
	}

	if (!dev->control) {
		btd_device_add_uuid(dev->btd_dev, AVRCP_REMOTE_UUID);
		if (!dev->control)
			goto drop;
	}

	control = dev->control;

	if (control->io) {
		error("Refusing unexpected connect from %s", address);
		goto drop;
	}

	avctp_set_state(control, AVCTP_STATE_CONNECTING);
	control->io = g_io_channel_ref(chan);

	if (audio_device_request_authorization(dev, AVRCP_TARGET_UUID,
						auth_cb, dev->control) < 0)
		goto drop;

	control->io_id = g_io_add_watch(chan, G_IO_ERR | G_IO_HUP | G_IO_NVAL,
							control_cb, control);
	return;

drop:
	if (!control || !control->io)
		g_io_channel_shutdown(chan, TRUE, NULL);
	if (control)
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
}

static GIOChannel *avctp_server_socket(const bdaddr_t *src, gboolean master)
{
	GError *err = NULL;
	GIOChannel *io;

	io = bt_io_listen(BT_IO_L2CAP, NULL, avctp_confirm_cb, NULL,
				NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, src,
				BT_IO_OPT_PSM, AVCTP_PSM,
				BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
				BT_IO_OPT_MASTER, master,
				BT_IO_OPT_INVALID);
	if (!io) {
		error("%s", err->message);
		g_error_free(err);
	}

	return io;
}

gboolean avrcp_connect(struct audio_device *dev)
{
	struct control *control = dev->control;
	GError *err = NULL;
	GIOChannel *io;

	if (control->state > AVCTP_STATE_DISCONNECTED)
		return TRUE;

	avctp_set_state(control, AVCTP_STATE_CONNECTING);

	io = bt_io_connect(BT_IO_L2CAP, avctp_connect_cb, control, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &dev->src,
				BT_IO_OPT_DEST_BDADDR, &dev->dst,
				BT_IO_OPT_PSM, AVCTP_PSM,
				BT_IO_OPT_INVALID);
	if (err) {
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
		error("%s", err->message);
		g_error_free(err);
		return FALSE;
	}

	control->io = io;

	return TRUE;
}

void avrcp_disconnect(struct audio_device *dev)
{
	struct control *control = dev->control;

	if (!(control && control->io))
		return;

	avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
}

int initialize_metadata()
{
	DBG ("initialize_meta_data");
	title = (char *)malloc(sizeof(char) * MAX_TITLE_LENGTH);
	if (!title)
		return -ENOMEM;

	strncpy(title, PTS_TITLE, MAX_TITLE_LENGTH);
	artist = (char *)malloc(sizeof(char) * MAX_ARTIST_LENGTH);
	if (!artist)
		return -ENOMEM;

	strncpy(artist, PTS_ARTIST, MAX_ARTIST_LENGTH);
	album = (char *)malloc(sizeof(char) * MAX_ALBUM_LENGTH);
	if (!album)
		return -ENOMEM;

	strncpy(album, PTS_ALBUM, MAX_ALBUM_LENGTH);
	no_of_tracks = (char *)malloc(sizeof(char) * DEFAULT_NO_OF_TRACKS_LEN);
	if (!no_of_tracks)
		return -ENOMEM;

	strcpy(no_of_tracks, DEFAULT_NO_OF_TRACKS);
	track_no = (char *)malloc(sizeof(char) * DEFAULT_TRACK_NO_LEN);
	if (!track_no)
		return -ENOMEM;

	strcpy(track_no, DEFAULT_TRACK_NO);
	duration = (char *)malloc(sizeof(char) * DEFAULT_DURATION_LEN);
	if (!duration)
		return -ENOMEM;

	strcpy(duration, DEFAULT_DURATION);
	DBG ("Done with initializing meta data");
	return 0;
}

void cleanup_metadata()
{
	DBG ("cleanup_metadata");
	if (title)
		free(title);
	if (artist)
		free(artist);
	if (album)
		free(album);
	if (no_of_tracks)
		free(no_of_tracks);
	if (track_no)
		free(track_no);
	if (duration)
		free(duration);
	DBG ("Done with clean up");
}

int avrcp_register(DBusConnection *conn, const bdaddr_t *src, GKeyFile *config)
{
	sdp_record_t *record;
	gboolean tmp, master = TRUE;
	GError *err = NULL;
	struct avctp_server *server;
	int str1, str2 ;

	if (config) {
		tmp = g_key_file_get_boolean(config, "General",
							"Master", &err);
		if (err) {
			DBG("audio.conf: %s", err->message);
			g_error_free(err);
		} else
			master = tmp;
		err = NULL;
		input_device_name = g_key_file_get_string(config,
			"AVRCP", "InputDeviceName", &err);
		if (err) {
			DBG("audio.conf: %s", err->message);
			input_device_name = NULL;
			g_error_free(err);
		}
		str1 = g_key_file_get_integer(config,"AVRCP",
				"MetaDataSupport", &err);
		if (err) {
			DBG ("audio.conf: %s", err->message);
			g_clear_error(&err);
		} else {
			metadata_support = str1;
			DBG ("the value of metadatasupport: %d", metadata_support);
		}
		str2 = g_key_file_get_integer(config,"AVRCP",
			"ControllerSupport", &err);
		if (err) {
			DBG ("audio.conf: %s", err->message);
			g_clear_error(&err);
		} else {
			controller_support = str2;
			DBG ("the value of controller_support: %d", controller_support);
		}

	}

	server = g_new0(struct avctp_server, 1);
	if (!server)
		return -ENOMEM;

	if (!connection)
		connection = dbus_connection_ref(conn);

	record = avrcp_tg_record();
	if (!record) {
		error("Unable to allocate new service record");
		g_free(server);
		return -1;
	}

	if (add_record_to_server(src, record) < 0) {
		error("Unable to register AVRCP target service record");
		g_free(server);
		sdp_record_free(record);
		return -1;
	}
	server->tg_record_id = record->handle;

	if (controller_support) {
		record = avrcp_ct_record();
		if (!record) {
			error("Unable to allocate new service record");
			return -1;
		}

		if (add_record_to_server(src, record) < 0) {
			error("Unable to register AVRCP controller service record");
			sdp_record_free(record);
			return -1;
		}
		server->ct_record_id = record->handle;
	}

	server->io = avctp_server_socket(src, master);
	if (!server->io) {
		if (controller_support) {
			remove_record_from_server(server->ct_record_id);
		}
		remove_record_from_server(server->tg_record_id);
		g_free(server);
		return -1;
	}

	bacpy(&server->src, src);

	servers = g_slist_append(servers, server);

	if (initialize_metadata() < 0) {
		error("Initializing meta data failed");
		return -1;
	}

	return 0;
}

static struct avctp_server *find_server(GSList *list, const bdaddr_t *src)
{
	GSList *l;

	for (l = list; l; l = l->next) {
		struct avctp_server *server = l->data;

		if (bacmp(&server->src, src) == 0)
			return server;
	}

	return NULL;
}

void avrcp_unregister(const bdaddr_t *src)
{
	struct avctp_server *server;

	server = find_server(servers, src);
	if (!server)
		return;

	servers = g_slist_remove(servers, server);

	if (controller_support) {
		remove_record_from_server(server->ct_record_id);
	}
	remove_record_from_server(server->tg_record_id);

	g_io_channel_shutdown(server->io, TRUE, NULL);
	g_io_channel_unref(server->io);
	g_free(server);
	cleanup_metadata();

	if (servers)
		return;

	dbus_connection_unref(connection);
	connection = NULL;
}

static DBusMessage *control_is_connected(DBusConnection *conn,
						DBusMessage *msg,
						void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	DBusMessage *reply;
	dbus_bool_t connected;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	connected = (control->state == AVCTP_STATE_CONNECTED);

	dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &connected,
					DBUS_TYPE_INVALID);

	return reply;
}

static int avctp_send_passthrough(struct control *control, uint8_t op)
{
	unsigned char buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH + 2];
	struct avctp_header *avctp = (void *) buf;
	struct avrcp_header *avrcp = (void *) &buf[AVCTP_HEADER_LENGTH];
	uint8_t *operands = &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	int err, sk = g_io_channel_unix_get_fd(control->io);
	static uint8_t transaction = 0;

	memset(buf, 0, sizeof(buf));

	avctp->transaction = transaction++;
	avctp->packet_type = AVCTP_PACKET_SINGLE;
	avctp->cr = AVCTP_COMMAND;
	avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

	avrcp->code = CTYPE_CONTROL;
	avrcp->subunit_type = SUBUNIT_PANEL;
	avrcp->opcode = OP_PASSTHROUGH;

	operands[0] = op & 0x7f;
	operands[1] = 0;

	err = write(sk, buf, sizeof(buf));
	if (err < 0)
		return err;

	/* Button release */
	avctp->transaction = transaction++;
	operands[0] |= 0x80;

	return write(sk, buf, sizeof(buf));
}

static int send_set_absolute_volume(struct control *control, uint8_t level)
{
	unsigned char buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH + 8];
	struct avctp_header *avctp = (void *) buf;
	struct avrcp_header *avrcp = (void *) &buf[AVCTP_HEADER_LENGTH];
	uint8_t *operands = &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	int packet_size = 0;
	int sk = g_io_channel_unix_get_fd(control->io);
	static uint8_t transaction = 0;

	DBG ("Register for volume changed event %d", level);
	level = level * ABSOLUTE_VOLUME_SCALE;
	DBG ("Absolute volume set is %d", level);
	memset(buf, 0, sizeof(buf));

	avctp->transaction = transaction++;
	avctp->packet_type = AVCTP_PACKET_SINGLE;
	avctp->cr = AVCTP_COMMAND;
	avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

	avrcp->code = CTYPE_CONTROL;
	avrcp->subunit_type = SUBUNIT_PANEL;
	avrcp->opcode = OP_VENDOR_DEP;

	operands[0] = 0x0;//BT SIG Company ID is 0x1958
	operands[1] = 0x19;
	operands[2] = 0x58;
	operands[3] = PDU_SET_ABSOLUTE_VOLUME;
	operands[4] = 0x0; //Reserved
	operands[5] = 0x0; //Parameter Len
	operands[6] = 0x1;
	operands[7] = ABSOLUTE_VOLUME(level);
	packet_size = 14;
	return write(sk, buf, packet_size);
}
static int send_event_changed(struct control *control, uint32_t event_id, uint32_t value)
{
	unsigned char buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH + 20];
	struct avctp_header *avctp = (void *) buf;
	struct avrcp_header *avrcp = (void *) &buf[AVCTP_HEADER_LENGTH];
	uint8_t *operands = &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	int err,packet_size = 0, loop;
	int sk = g_io_channel_unix_get_fd(control->io);
	static uint8_t transaction = 0;
	uint32_t *identifier = (void *)&buf[AVCTP_HEADER_LENGTH +
				AVRCP_HEADER_LENGTH+12];

	memset(buf, 0, sizeof(buf));

	avctp->transaction = transaction++;
	avctp->packet_type = AVCTP_PACKET_SINGLE;
	avctp->cr = AVCTP_RESPONSE;
	avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

	avrcp->code = CTYPE_CHANGED;
	avrcp->subunit_type = SUBUNIT_PANEL;
	avrcp->opcode = OP_VENDOR_DEP;

	operands[0] = 0x0;//Company ID
	operands[1] = 0x19;
	operands[2] = 0x58;
	operands[3] = PDU_REGISTER_NOTIFICATION;
	operands[4] = 0x0; //Reserved

	if (event_id == EVENT_PLAYBACK_STATUS_CHANGED) {
		current_status = value;
		if (event_playback_status == FALSE) {
			return 0;
		}
		operands[5] = 0x0; //Parameter Len
		operands[6] = 0x2;
		operands[7] = EVENT_PLAYBACK_STATUS_CHANGED;
		switch (value) {
		case 0:
			operands[8] = STATUS_STOPPED;
			break;
		case 1:
			operands[8] = STATUS_PLAYING;
			break;
		case 2:
			operands[8] = STATUS_PAUSED;
			break;
		case 3:
			operands[8] = STATUS_FWD_SEEK;
			break;
		case 4:
			operands[8] = STATUS_REV_SEEK;
			break;
		default:
			operands[8] = STATUS_ERROR;
			break;
		}
		packet_size = 15;
		avctp->transaction = transaction_playback_event;
		event_playback_status = FALSE;

	} else if (event_id == EVENT_TRACK_CHANGED) {
		if (event_track_changed == FALSE) {
			return 0;
		}
		operands[5] = 0x0; //Parameter Len
		operands[6] = 0x9;
		operands[7] = EVENT_TRACK_CHANGED;
		operands[8] = 0x0;
		operands[9] = 0x0;
		operands[10] = 0x0;
		operands[11] = 0x0;
		identifier[0] = htonl(value);
		packet_size = 22;
		avctp->transaction = transaction_track_event;
		event_track_changed = FALSE;
	} else
		return 0;

	return write(sk, buf, packet_size);
}

static int send_play_status(struct control *control,uint32_t duration,
				uint32_t position, uint32_t status)
{
	unsigned char buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH + 16];
	struct avctp_header *avctp = (void *) buf;
	struct avrcp_header *avrcp = (void *) &buf[AVCTP_HEADER_LENGTH];
	uint8_t *operands = &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	int err,packet_size = 0, loop;
	int sk = g_io_channel_unix_get_fd(control->io);
	static uint8_t transaction = 0;
	uint32_t *identifier = (void *)&buf[AVCTP_HEADER_LENGTH +
				AVRCP_HEADER_LENGTH+7];

	memset(buf, 0, sizeof(buf));
	current_status = status;
	avctp->transaction = play_status_transaction_label;
	avctp->packet_type = AVCTP_PACKET_SINGLE;
	avctp->cr = AVCTP_RESPONSE;
	avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

	avrcp->code = CTYPE_STABLE;
	avrcp->subunit_type = SUBUNIT_PANEL;
	avrcp->opcode = OP_VENDOR_DEP;

	operands[0] = 0x0;//Company ID
	operands[1] = 0x19;
	operands[2] = 0x58;
	operands[3] = PDU_GET_PLAY_STATUS;
	operands[4] = 0x0; //Reserved
	operands[5] = 0x0; //Parameter Len
	operands[6] = 0x9;

	//Converting from little Endian to Big Endian.
	identifier[0] = htonl(duration);
	identifier[1] = htonl(position);
	operands[15] = status;

	packet_size = 22;

	return write(sk, buf, packet_size);
}

static DBusMessage *volume_up(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	int err;

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotConnected",
					"Device not Connected");

	if (!control->target)
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotSupported",
					"AVRCP Target role not supported");

	err = avctp_send_passthrough(control, VOL_UP_OP);
	if (err < 0)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".Failed",
							strerror(-err));

	return dbus_message_new_method_return(msg);
}

static DBusMessage *volume_down(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	int err;

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotConnected",
					"Device not Connected");

	if (!control->target)
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotSupported",
					"AVRCP Target role not supported");

	err = avctp_send_passthrough(control, VOL_DOWN_OP);
	if (err < 0)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".Failed",
							strerror(-err));

	return dbus_message_new_method_return(msg);
}

static DBusMessage *notify_event_changed(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	int err;
	uint16_t event_id = 0;
	const gchar * value = NULL;

	if (!dbus_message_get_args(msg,NULL,
					DBUS_TYPE_UINT32,&event_id,
					DBUS_TYPE_UINT32,&value,
					DBUS_TYPE_INVALID)) {
		return NULL;
	}

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
						ERROR_INTERFACE ".NotConnected",
						"Device not Connected");

	err = send_event_changed(control,event_id,value);
	if (err < 0)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".Failed",
						strerror(-err));

	return dbus_message_new_method_return(msg);
}

static DBusMessage *notify_play_status(DBusConnection *conn, DBusMessage *msg,
                                                                void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	int err;
	uint32_t status = 0,duration = 0,position = 0;

	if (!dbus_message_get_args(msg,NULL,
				DBUS_TYPE_UINT32,&duration,
				DBUS_TYPE_UINT32,&position,
				DBUS_TYPE_UINT32,&status,
				DBUS_TYPE_INVALID)) {
		return NULL;
	}

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotConnected",
					"Device not Connected");

	err = send_play_status(control,duration,position,status);
	if (err < 0)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".Failed",
						strerror(-err));

	return dbus_message_new_method_return(msg);
}

static DBusMessage *set_absolute_volume(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	int err = -1;
	uint8_t level = 0;

	if (!dbus_message_get_args(msg,NULL,
					DBUS_TYPE_UINT16,&level,
					DBUS_TYPE_INVALID)) {
		return NULL;
	}
	DBG ("set_absolute_volume %d", level);

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
						ERROR_INTERFACE ".NotConnected",
						"Device not Connected");

	if (!control->target_cat2_support) {
		error("Remote device do not support AVRCP Target cat-2");
		return g_dbus_create_error(msg,
						ERROR_INTERFACE ".NotSupported",
						"AVRCP Target cat-2 role is not supported");
    }

	err = send_set_absolute_volume(control,level);
	if (err < 0)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".Failed",
						strerror(-err));

	return dbus_message_new_method_return(msg);
}

static DBusMessage *register_volume_changed_event(DBusConnection *conn,
								DBusMessage *msg,
								void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	int err;

	DBG ("register_volume_changed_event");

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
						ERROR_INTERFACE ".NotConnected",
						"Device not Connected");

	if (!control->target_cat2_support) {
		error("Remote device do not support AVRCP Target cat-2");
		return g_dbus_create_error(msg,
						ERROR_INTERFACE ".NotSupported",
						"AVRCP Target cat-2 role is not supported");
	}

	err = send_register_volume_changed_event(control);
	if (err < 0)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".Failed",
						strerror(-err));

	return dbus_message_new_method_return(msg);
}

static int send_register_volume_changed_event(struct control *control)
{
	unsigned char buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH + 12];
	struct avctp_header *avctp = (void *) buf;
	struct avrcp_header *avrcp = (void *) &buf[AVCTP_HEADER_LENGTH];
	uint8_t *operands = &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	int packet_size = 0;
	int sk = g_io_channel_unix_get_fd(control->io);
	static uint8_t transaction = 1;

	DBG ("Register for volume changed event");
	memset(buf, 0, sizeof(buf));

	avctp->transaction = transaction++;
	avctp->packet_type = AVCTP_PACKET_SINGLE;
	avctp->cr = AVCTP_COMMAND;
	avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

	avrcp->code = CTYPE_NOTIFY;
	avrcp->subunit_type = SUBUNIT_PANEL;
	avrcp->opcode = OP_VENDOR_DEP;

	operands[0] = 0x0;//Company ID
	operands[1] = 0x19;
	operands[2] = 0x58;
	operands[3] = PDU_REGISTER_NOTIFICATION;
	operands[4] = 0x0; //Reserved
	operands[5] = 0x0; //Parameter Len
	operands[6] = 0x5;
	operands[7] = EVENT_VOLUME_CHANGED;
	operands[8] = 0x0; //Reserved
	operands[9] = 0x0;
	operands[10] = 0x0;
	operands[11] = 0x0;
	packet_size = 18;
	return write(sk, buf, packet_size);
}


int send_meta_data_continue_response(struct control *control, int transaction_label)
{
	unsigned char buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH +
				META_DATA_PARAMS_LENGTH + remaning_meta_data_len];
	struct avctp_header *avctp = (void *) buf;
	struct avrcp_header *avrcp = (void *) &buf[AVCTP_HEADER_LENGTH];
	struct meta_data_params *mdp =
	(void *) &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	int sk = g_io_channel_unix_get_fd(control->io);
	int total_len = 0;
	char *tmp = NULL;
	uint8_t *operands = NULL;

	memset(buf, 0, sizeof(buf));
	avctp->transaction = transaction_label;
	avctp->packet_type = AVCTP_PACKET_SINGLE;
	avctp->cr = AVCTP_RESPONSE;
	avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

	avrcp->code = CTYPE_STABLE;
	avrcp->subunit_type = SUBUNIT_PANEL;
	avrcp->opcode = OP_VENDOR_DEP;

	mdp->pduid = PDU_GET_ELEMENT_ATTRIBUTES;
	mdp->reserved = 0x0;
	mdp->pktType = AVRCP_PACKET_END;

	//Company Id
	operands = &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	operands[0] = 0x00;
	operands[1] = 0x19;
	operands[2] = 0x58;

	DBG ("remanint = %s, len = %d",remaning_meta_data,remaning_meta_data_len);

	tmp = (char *)&buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH +
					META_DATA_PARAMS_LENGTH - 1];

	memcpy(tmp, remaning_meta_data, remaning_meta_data_len);

	DBG ("Buffer len is %d",strlen(buf));

	total_len = AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH
			+ META_DATA_PARAMS_LENGTH + remaning_meta_data_len - 1;

	if ((total_len - AVCTP_HEADER_LENGTH) > AVRCP_MAX_PKT_SIZE) {
		mdp->paramLen = htons(AVRCP_MAX_PKT_SIZE -
				(AVRCP_HEADER_LENGTH + META_DATA_PARAMS_LENGTH - 1));
		mdp->pktType = AVRCP_PACKET_CONTINUE;
		write(sk, buf, (AVRCP_MAX_PKT_SIZE + AVCTP_HEADER_LENGTH));
		if (remaning_meta_data != NULL) {
			free(remaning_meta_data);
			remaning_meta_data = NULL;
		}
		remaning_meta_data = (char *)malloc(sizeof(char)*
				(total_len - AVRCP_MAX_PKT_SIZE - AVCTP_HEADER_LENGTH));
		if (remaning_meta_data == NULL) {
			avctp->cr = AVCTP_RESPONSE;
			avrcp->code = CTYPE_REJECTED;
			mdp->pktType = AVRCP_PACKET_SINGLE;
			mdp->paramLen = htons(0x1);
			mdp->capId = ERROR_INTERNAL;
			total_len = AVCTP_HEADER_LENGTH
					+ AVRCP_HEADER_LENGTH + META_DATA_PARAMS_LENGTH;
			return write(sk, buf, total_len);
		}
		remaning_meta_data_len = total_len - AVRCP_MAX_PKT_SIZE - AVCTP_HEADER_LENGTH;
		memcpy(remaning_meta_data, &(buf[AVRCP_MAX_PKT_SIZE + AVCTP_HEADER_LENGTH]),
				(total_len - AVRCP_MAX_PKT_SIZE - AVCTP_HEADER_LENGTH));
		return 0;
	} else {
		mdp->pktType = AVRCP_PACKET_END;
		mdp->paramLen =
		htons(total_len - (AVCTP_HEADER_LENGTH +
							AVRCP_HEADER_LENGTH + META_DATA_PARAMS_LENGTH - 1));
		if (remaning_meta_data != NULL) {
			free(remaning_meta_data);
			remaning_meta_data = NULL;
		}
		remaning_meta_data_len = 0;
		info("End buf len %d",strlen(buf));
		return write(sk, buf, total_len);
	}
}

int send_meta_data(struct control *control,int transaction_label,
			uint8_t att_count,uint8_t meta_data_mask)
{
	unsigned char buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH +
					META_DATA_PARAMS_LENGTH + (MEDIA_ATTR_LEN *6) +
					strlen(title)+ strlen(artist)+ strlen(album)+
					 strlen(no_of_tracks)+ strlen(track_no)+ strlen(duration)];
	struct avctp_header *avctp = (void *) buf;
	struct avrcp_header *avrcp = (void *) &buf[AVCTP_HEADER_LENGTH];
	struct media_attr *ma = NULL;
	int len = 0,total_len = 0, meta_param_len = 0,index = 0;
	uint8_t *operands = &buf[AVCTP_HEADER_LENGTH+AVRCP_HEADER_LENGTH +
				META_DATA_PARAMS_LENGTH-1];
	struct meta_data_params *mdp = (void *) &buf[AVCTP_HEADER_LENGTH +
							AVRCP_HEADER_LENGTH];
	int sk = g_io_channel_unix_get_fd(control->io);
	char *tmp = NULL;
	gboolean first_element = TRUE;


	memset(buf, 0, sizeof(buf));
	avctp->transaction = transaction_label;
	avctp->packet_type = AVCTP_PACKET_SINGLE;
	avctp->cr = AVCTP_RESPONSE;
	avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

	avrcp->code = CTYPE_STABLE;
	avrcp->subunit_type = SUBUNIT_PANEL;
	avrcp->opcode = OP_VENDOR_DEP;

	mdp->pduid = PDU_GET_ELEMENT_ATTRIBUTES;
	mdp->reserved = 0x0;
	mdp->pktType = AVRCP_PACKET_SINGLE;

	if((meta_data_mask & META_DATA_GENRE) != 0x0)
		operands[0] = att_count - 1;
	else
		operands[0] = att_count; //Current Supported Metadata Count

	//Company Id
	operands = &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	operands[0] = 0x00;
	operands[1] = 0x19;
	operands[2] = 0x58;
	total_len = AVCTP_HEADER_LENGTH+AVRCP_HEADER_LENGTH +
			META_DATA_PARAMS_LENGTH;
	ma = (void *)&buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH
						+ META_DATA_PARAMS_LENGTH];

	if((meta_data_mask & META_DATA_TITLE) != 0x0) {
		operands = (uint8_t *)ma;
		if (first_element == FALSE)
			operands+= MEDIA_ATTR_LEN+len;
		else
			first_element = FALSE;

		ma->att_id = htonl(0x1);
		ma->char_set_id = htons(0x6A);
		len = strlen(title);
		ma->att_val_len = htons(len);
		strcpy(ma->att_val,title);
		total_len = total_len+MEDIA_ATTR_LEN+len;
	}
	if((meta_data_mask & META_DATA_ARTIST) != 0x0) {
		operands = (uint8_t *)ma;
		if (first_element == FALSE)
			operands+= MEDIA_ATTR_LEN+len;
		else
			first_element = FALSE;
		ma = (struct media_attr *)operands;
		ma->att_id = htonl(0x2);
		ma->char_set_id = htons(0x6A);
		len = strlen(artist);
		ma->att_val_len = htons(len);
		strcpy(ma->att_val,artist);
		total_len = total_len+MEDIA_ATTR_LEN+len;
	}
	if((meta_data_mask & META_DATA_ALBUM) != 0x0) {
		operands = (uint8_t *)ma;
		if (first_element == FALSE)
			operands+= MEDIA_ATTR_LEN+len;
		else
			first_element = FALSE;

		ma = (struct media_attr *)operands;
		ma->att_id = htonl(0x3);
		ma->char_set_id = htons(0x6A);
		len = strlen(album);
		ma->att_val_len = htons(len);
		strcpy(ma->att_val,album);
		total_len = total_len+MEDIA_ATTR_LEN+len;
	}
	if((meta_data_mask & META_DATA_TOTAL_NUMBER_OF_MEDIA) != 0x0) {
		operands = (uint8_t *)ma;
		if (first_element == FALSE)
			operands+= MEDIA_ATTR_LEN+len;
		else
			first_element = FALSE;

		ma = (struct media_attr *)operands;
		ma->att_id = htonl(0x5);
		ma->char_set_id = htons(0x6A);
		len = strlen(no_of_tracks);
		ma->att_val_len = htons(len);
		strcpy(ma->att_val,no_of_tracks);
		total_len = total_len+MEDIA_ATTR_LEN+len;
	}
	if((meta_data_mask & META_DATA_NUMBER_OF_MEDIA) != 0x0) {
		operands = (uint8_t *)ma;
		if (first_element == FALSE)
			operands+= MEDIA_ATTR_LEN+len;
		else
			first_element = FALSE;

		ma = (struct media_attr *)operands;
		ma->att_id = htonl(0x4);
		ma->char_set_id = htons(0x6A);
		len = strlen(track_no);
		ma->att_val_len = htons(len);
		strcpy(ma->att_val,track_no);
		total_len = total_len+MEDIA_ATTR_LEN+len;
	}
	if((meta_data_mask & META_DATA_PLAYING_TIME) != 0x0) {
		operands = (uint8_t *)ma;
		if (first_element == FALSE)
			operands+= MEDIA_ATTR_LEN+len;
		else
			first_element = FALSE;

		ma = (struct media_attr *)operands;
		ma->att_id = htonl(0x7);
		ma->char_set_id = htons(0x6A);
		len = strlen(duration);
		ma->att_val_len = htons(len);
		strcpy(ma->att_val,duration);
		total_len = total_len+MEDIA_ATTR_LEN+len;
	}
	if ((total_len - AVCTP_HEADER_LENGTH) > AVRCP_MAX_PKT_SIZE) {
		mdp->paramLen = htons(AVRCP_MAX_PKT_SIZE - (AVRCP_HEADER_LENGTH
				+ META_DATA_PARAMS_LENGTH - 1));
		mdp->pktType = AVRCP_PACKET_START;
		write(sk, buf, (AVRCP_MAX_PKT_SIZE + AVCTP_HEADER_LENGTH));
		info("sending start pkt");
		remaning_meta_data_len = total_len - AVRCP_MAX_PKT_SIZE - AVCTP_HEADER_LENGTH;
		tmp = (char *)&(buf[AVRCP_MAX_PKT_SIZE + AVCTP_HEADER_LENGTH]);
		remaning_meta_data = (char *)malloc(sizeof(char)*
			(total_len - AVRCP_MAX_PKT_SIZE - AVCTP_HEADER_LENGTH));
		if (remaning_meta_data == NULL) {
			avctp->cr = AVCTP_RESPONSE;
			avrcp->code = CTYPE_REJECTED;
			mdp->pktType = AVRCP_PACKET_SINGLE;
			mdp->paramLen = htons(0x1);
			mdp->capId = ERROR_INTERNAL;
			total_len = AVCTP_HEADER_LENGTH
					+ AVRCP_HEADER_LENGTH + META_DATA_PARAMS_LENGTH;
			return write(sk, buf, total_len);
		}
		memcpy(remaning_meta_data, tmp,
				(total_len - AVRCP_MAX_PKT_SIZE - AVCTP_HEADER_LENGTH));

		DBG ("remaning_meta_data %s, %d",remaning_meta_data, strlen(remaning_meta_data));

		return 0;
	} else {
		mdp->pktType = AVRCP_PACKET_SINGLE;
		mdp->paramLen = htons(total_len - (AVCTP_HEADER_LENGTH
				+ AVRCP_HEADER_LENGTH + META_DATA_PARAMS_LENGTH - 1));
		return write(sk, buf, total_len);
	}
}

static DBusMessage *notify_meta_data_changed(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	DBusMessage *reply;
	int err;
	const char *c_title = NULL;
	const char *c_artist = NULL;
	const char *c_album = NULL;
	const char *c_no_of_tracks = NULL;
	const char *c_track_no = NULL;
	const char *c_duration = NULL;

	if (!dbus_message_get_args(msg, NULL,
				DBUS_TYPE_STRING, &c_title,
				DBUS_TYPE_STRING, &c_artist,
				DBUS_TYPE_STRING, &c_album,
				DBUS_TYPE_STRING, &c_no_of_tracks,
				DBUS_TYPE_STRING, &c_track_no,
				DBUS_TYPE_STRING, &c_duration,
				DBUS_TYPE_INVALID)) {
		return NULL;
	}
	strcpy(title, c_title);
	strcpy(artist, c_artist);
	strcpy(album, c_album);
	strcpy(no_of_tracks, c_no_of_tracks);
	strcpy(track_no, c_track_no);
	strcpy(duration, c_duration);

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	return dbus_message_new_method_return(msg);
}

static DBusMessage *control_get_properties(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device = data;
	DBusMessage *reply;
	DBusMessageIter iter;
	DBusMessageIter dict;
	gboolean value;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	dbus_message_iter_init_append(reply, &iter);

	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	/* Connected */
	value = (device->control->state == AVCTP_STATE_CONNECTED);
	dict_append_entry(&dict, "Connected", DBUS_TYPE_BOOLEAN, &value);

	dbus_message_iter_close_container(&iter, &dict);

	return reply;
}

static GDBusMethodTable control_methods[] = {
	{ "IsConnected",	"",	"b",	control_is_connected,
						G_DBUS_METHOD_FLAG_DEPRECATED },
	{ "GetProperties",	"",	"a{sv}",control_get_properties },
	{ "VolumeUp",		"",	"",	volume_up },
	{ "VolumeDown",		"",	"",	volume_down },
	{ "NotifyMetaDataChange","ssssss","",	notify_meta_data_changed },
	{ "NotifyEventChange",	"uu",	"",	notify_event_changed },
	{ "NotifyPlayStatus",	"uuu",	"",	notify_play_status },
	{ "SetAbsoluteVolume",	"q",	"",	set_absolute_volume },
	{ "RegisterVolumeChangedEvent",	"",	"",	register_volume_changed_event },
	{ NULL, NULL, NULL, NULL }
};

static GDBusSignalTable control_signals[] = {
	{ "Connected",			"",	G_DBUS_SIGNAL_FLAG_DEPRECATED},
	{ "Disconnected",		"",	G_DBUS_SIGNAL_FLAG_DEPRECATED},
	{ "PropertyChanged",		"sv"	},
	{ "PlayStatusRequested",	""	},
	{ "AbsoluteVolumeChanged",	"q"	},
	{ NULL, NULL }
};

static void path_unregister(void *data)
{
	struct audio_device *dev = data;
	struct control *control = dev->control;

	DBG("Unregistered interface %s on path %s",
		AUDIO_CONTROL_INTERFACE, dev->path);

	if (control->state != AVCTP_STATE_DISCONNECTED)
		avctp_disconnected(dev);

	g_free(control);
	dev->control = NULL;
}

void control_unregister(struct audio_device *dev)
{
	g_dbus_unregister_interface(dev->conn, dev->path,
		AUDIO_CONTROL_INTERFACE);
}

void control_update(struct audio_device *dev, uint16_t uuid16)
{
	struct control *control = dev->control;

	if (uuid16 == AV_REMOTE_TARGET_SVCLASS_ID)
		control->target = TRUE;
}

struct control *control_init(struct audio_device *dev, uint16_t uuid16)
{
	struct control *control;

	if (!g_dbus_register_interface(dev->conn, dev->path,
					AUDIO_CONTROL_INTERFACE,
					control_methods, control_signals, NULL,
					dev, path_unregister))
		return NULL;

	DBG("Registered interface %s on path %s",
		AUDIO_CONTROL_INTERFACE, dev->path);

	control = g_new0(struct control, 1);
	control->dev = dev;
	control->state = AVCTP_STATE_DISCONNECTED;
	control->uinput = -1;

	if (uuid16 == AV_REMOTE_TARGET_SVCLASS_ID)
		control->target = TRUE;

	return control;
}

gboolean control_is_active(struct audio_device *dev)
{
	struct control *control = dev->control;

	if (control && control->state != AVCTP_STATE_DISCONNECTED)
		return TRUE;

	return FALSE;
}

unsigned int avctp_add_state_cb(avctp_state_cb cb, void *user_data)
{
	struct avctp_state_callback *state_cb;
	static unsigned int id = 0;

	state_cb = g_new(struct avctp_state_callback, 1);
	state_cb->cb = cb;
	state_cb->user_data = user_data;
	state_cb->id = ++id;

	avctp_callbacks = g_slist_append(avctp_callbacks, state_cb);

	return state_cb->id;
}

gboolean avctp_remove_state_cb(unsigned int id)
{
	GSList *l;

	for (l = avctp_callbacks; l != NULL; l = l->next) {
		struct avctp_state_callback *cb = l->data;
		if (cb && cb->id == id) {
			avctp_callbacks = g_slist_remove(avctp_callbacks, cb);
			g_free(cb);
			return TRUE;
		}
	}

	return FALSE;
}

static int absolute_volume(int level) {
	float abf_level = level / ABSOLUTE_VOLUME_SCALE;
	int ab_level = abf_level;
	if((abf_level - ab_level) > 0.5)
		return (ab_level + 1);
	else
		return ab_level;
}
