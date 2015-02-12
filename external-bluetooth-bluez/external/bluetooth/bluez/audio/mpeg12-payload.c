/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Motorola Corporation
 *  Copyright (C) 2011  Ittiam Systems (P) Ltd
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* System include files */
#include <stdio.h>
#include <malloc.h>
#include <string.h>

/* User include files */
#include <utils/Log.h>
#include "mp3-encode.h"
#include "mpeg12-payload.h"

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DBG LOGD
#else
#define DBG(fmt, arg...)
#endif

#define ERR LOGE
/*****************************************************************************/
/* Static Function Declarations                                              */
/*****************************************************************************/

static void cb_mem_copy(void *cb_params, void *src, void *dst, UWORD32 length);
static void *cb_mem_alloc(void *cb_params, UWORD32 alloc_size);
static void cb_mem_free(void *cb_params, void *mem_ptr);

/*****************************************************************************/
/*  Function Name : cb_mem_free                                              */
/*                                                                           */
/*  Description   : Callback function implementation for MPEG12PL for        */
/*                  freeing allocated memory                                 */
/*****************************************************************************/
void cb_mem_free(void *cb_params, void *mem_ptr)
{
	free(mem_ptr);
	return;
}

/*****************************************************************************/
/*  Function Name : cb_mem_alloc                                             */
/*                                                                           */
/*  Description   : Callback function implementation for MPEG12PL for memory */
/*                  allocation                                               */
/*****************************************************************************/

void *cb_mem_alloc(void *cb_params, UWORD32 alloc_size)
{
	return malloc(alloc_size);
}

/*****************************************************************************/
/*  Function Name : cb_mem_copy                                              */
/*                                                                           */
/*  Description   : Wrapper for memcpy, used as a callback to MPEG12PL       */
/*****************************************************************************/
void cb_mem_copy(void *cb_params, void *src, void *dst, UWORD32 length)
{
	memcpy(dst, src, length);
}

int ia_mpeg12pl_rtp_init(void **handle, mpeg12pl_attr_t *pAttr,
					mpeg12pl_cb_funcs_t *pCb_funcs,
					mpeg12pl_packet_t *pPacket)
{
	void *g_server_handle;
	int ret = 0;
	WORD32 in_status = 0;
	WORD32 out_status = 0;
	MPEG12PL_PAYLOAD_TYPE_T payload_type = MPEG12PL_MPEG12_AUDIO;

	/* Set sequence number to some arbitary number */
	pPacket->seq_num = 1000;

	/* Allocate memory for handle */
	g_server_handle = malloc(MPEG12PL_HANDLE_SIZE);

	if (NULL == g_server_handle) {
		ERR("Could not allocate g_server_handle");
		return -1;
	}

	/* Set attribute values */
	pAttr->max_payload_len = 882; //link->mtu - RTP_HEADER_SIZE
	pAttr->payload_type    = payload_type;

	pAttr->du_mode    = MPEG12PL_DU_PACKET;
	pAttr->usage_mode = MPEG12PL_SINGLE_AU_FRAG;

	/* Initialize the library */
	ret = mpeg12pl_init(g_server_handle, pAttr);

	if (MPEG12PL_SUCCESS != ret) {
		ERR("mpeg12 payload Server Initialization failed ");
		return -1;
	} else {
		DBG("mpeg12 payload Server Initialization successful ");
	}

	pCb_funcs->mpeg12pl_mem_copy  = cb_mem_copy; //memcpy
	pCb_funcs->mpeg12pl_cm_params = 0;
	pCb_funcs->mpeg12pl_al_params = 0;
	pCb_funcs->mpeg12pl_alloc     = cb_mem_alloc;//malloc
	pCb_funcs->mpeg12pl_fr_params = 0;
	pCb_funcs->mpeg12pl_free      = cb_mem_free;//free;

	ret = mpeg12pl_register_callbacks(g_server_handle, pCb_funcs);

	/* Register callbacks */
	if(MPEG12PL_SUCCESS != ret) {
		ERR("mpeg12 payload Server Registering callbacks failed ");
		return -1;
	} else {
		DBG("mpeg12 payload Server Registering callbacks Successful.");
	}

	/* Allocate memory for packet */
	pPacket->payload_ptr = malloc(pAttr->max_payload_len);
	if (NULL == pPacket->payload_ptr) {
		ERR("Could not allocate pPacket->payload_ptr");
		return -1;
	}

	*handle = g_server_handle;

	return  ret;
}



int ia_mpeg12pl_add_rtp_header(void *handle, UWORD8* input_buf1,
					UWORD32 length,
					mpeg12pl_packet_t *pPacket)
{
	int ret = 0;
	mpeg12pl_audio_params_t in_audio_params;
	void *g_server_handle = handle;

	if(NULL == g_server_handle) {
		ERR("Could not get valid  g_server_handle");
		return -1;
	}

	/* Input payload parameters */
	in_audio_params.bitstream_ptr = input_buf1;
	in_audio_params.bitstream_len = length;

	/* Add header to the payload */
	ret = mpeg12pl_add_header(g_server_handle,
					&in_audio_params, pPacket);

	return ret;
}

int ia_mpeg12pl_delete(void *handle)
{
	free(handle); //free server handle
	return 0;
}
