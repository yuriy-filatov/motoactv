/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2001-2002  Nokia Corporation
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2002-2003  Stephen Crane <steve.crane@rococosoft.com>
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

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>

#ifdef SDP_DEBUG
#include <syslog.h>
#define SDPDBG(fmt, arg...) syslog(LOG_DEBUG, "%s: " fmt "\n", __func__ , ## arg)
#else
#define SDPDBG(fmt...)
#endif

#define EIR_DATA_LENGTH  240

#define EIR_FLAGS                   0x01  /* flags */
#define EIR_UUID16_SOME             0x02  /* 16-bit UUID, more available */
#define EIR_UUID16_ALL              0x03  /* 16-bit UUID, all listed */
#define EIR_UUID32_SOME             0x04  /* 32-bit UUID, more available */
#define EIR_UUID32_ALL              0x05  /* 32-bit UUID, all listed */
#define EIR_UUID128_SOME            0x06  /* 128-bit UUID, more available */
#define EIR_UUID128_ALL             0x07  /* 128-bit UUID, all listed */
#define EIR_NAME_SHORT              0x08  /* shortened local name */
#define EIR_NAME_COMPLETE           0x09  /* complete local name */
#define EIR_TX_POWER                0x0A  /* transmit power level */
#define EIR_DEVICE_ID               0x10  /* device ID */

typedef struct request {
	bdaddr_t device;
	bdaddr_t bdaddr;
	int      local;
	int      sock;
	int      mtu;
	int      flags;
	uint8_t  *buf;
	int      len;
} sdp_req_t;

void handle_request(int sk, uint8_t *data, int len);

int service_register_req(sdp_req_t *req, sdp_buf_t *rsp);
int service_update_req(sdp_req_t *req, sdp_buf_t *rsp);
int service_remove_req(sdp_req_t *req, sdp_buf_t *rsp);

void register_public_browse_group(void);
void register_server_service(void);
void register_device_id(const uint16_t spec, const uint16_t vendor,
		const uint16_t product, const uint16_t version,
		const uint8_t primary, const uint16_t source);

int record_sort(const void *r1, const void *r2);
void sdp_svcdb_reset(void);
void sdp_svcdb_collect_all(int sock);
void sdp_svcdb_set_collectable(sdp_record_t *rec, int sock);
void sdp_svcdb_collect(sdp_record_t *rec);
sdp_record_t *sdp_record_find(uint32_t handle);
void sdp_record_add(const bdaddr_t *device, sdp_record_t *rec);
int sdp_record_remove(uint32_t handle);
sdp_list_t *sdp_get_record_list(void);
sdp_list_t *sdp_get_access_list(void);
int sdp_check_access(uint32_t handle, bdaddr_t *device);
uint32_t sdp_next_handle(void);

uint32_t sdp_get_time();

#define SDP_SERVER_COMPAT (1 << 0)
#define SDP_SERVER_MASTER (1 << 1)

int start_sdp_server(uint16_t mtu, const char *did, uint32_t flags);
void stop_sdp_server(void);

int add_record_to_server(const bdaddr_t *src, sdp_record_t *rec);
int remove_record_from_server(uint32_t handle);

static inline int android_get_control_socket(const char *name);
void sdp_init_services_list(bdaddr_t *device);
