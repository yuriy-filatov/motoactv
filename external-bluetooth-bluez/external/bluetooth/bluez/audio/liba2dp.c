/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2007  Nokia Corporation
 *  Copyright (C) 2004-2008  Marcel Holtmann <marcel@holtmann.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/prctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#include "ipc.h"
#include "sbc.h"
#include "rtp.h"
#include "liba2dp.h"

#define LOG_NDEBUG 0
#define LOG_TAG "A2DP"
#include <utils/Log.h>

#ifdef ENABLE_MP3_ITTIAM_CODEC
#include "mp3-encode.h"

#ifdef ADD_RTP_PAYLOAD_HEADER
#include "mpeg12-payload.h"
#endif //ADD_RTP_PAYLOAD_HEADER

#endif //ENABLE_MP3_ITTIAM_CODEC

#ifdef ENABLE_CSR_APTX_CODEC
#include "aptXbtenc.h"
#endif //ENABLE_CSR_APTX_CODEC

#define ENABLE_DEBUG
/* #define ENABLE_VERBOSE */
/* #define ENABLE_TIMING */

#ifdef ENABLE_DEBUG
#define DBG LOGD
#else
#define DBG(fmt, arg...)
#endif

#ifdef ENABLE_VERBOSE
#define VDBG LOGV
#else
#define VDBG(fmt, arg...)
#endif

#ifndef MIN
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
# define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#define MAX_BITPOOL 53
#define MIN_BITPOOL 2

#define ERR LOGE

/* Number of packets to buffer in the stream socket */
#define PACKET_BUFFER_COUNT		10

/* timeout in milliseconds to prevent poll() from hanging indefinitely */
#define POLL_TIMEOUT			1000

/* milliseconds of unsucessfull a2dp packets before we stop trying to catch up
 * on write()'s and fall-back to metered writes */
#define CATCH_UP_TIMEOUT		200

/* timeout in milliseconds for a2dp_write */
#define WRITE_TIMEOUT			1000

/* timeout in seconds for command socket recv() */
#define RECV_TIMEOUT			5

#ifdef ENABLE_MP3_ITTIAM_CODEC
#define RTP_PAYLOAD_SIZE 13
//MP3 codec requires output buffer size of 2048 exclusive of RTP_PAYLOAD_SIZE
#define BUFFER_SIZE 2048 + RTP_PAYLOAD_SIZE
#define MP3_BUFFER_SIZE 10240*2
#define IAUDIO_1_0 0	 /**< Mono. */
#define IAUDIO_2_0 1	/**< Stereo. */
#else
#define BUFFER_SIZE 2048
#endif //ENABLE_MP3_ITTIAM_CODEC

#ifdef ENABLE_CSR_APTX_CODEC
#define BUFFER_SIZE 4096
#endif

#ifdef ENABLE_CSR_APTX_CODEC
/* btaptx local data stracture
 * btaptx_struct could be removed as we are not using it....however, 
 * it is just to keep symmetry with SBC implementation as much as possible. */

struct btaptx_struct {
	unsigned long flags;
	int frequency;
	int mode;
	int endian;
};
typedef struct btaptx_struct btaptx_t;
#endif //ENABLE_CSR_APTX_CODEC

typedef enum {
	A2DP_STATE_NONE = 0,
	A2DP_STATE_INITIALIZED,
	A2DP_STATE_CONFIGURING,
	A2DP_STATE_CONFIGURED,
	A2DP_STATE_STARTING,
	A2DP_STATE_STARTED,
	A2DP_STATE_STOPPING,
} a2dp_state_t;

typedef enum {
	A2DP_CMD_NONE = 0,
	A2DP_CMD_INIT,
	A2DP_CMD_CONFIGURE,
	A2DP_CMD_START,
	A2DP_CMD_STOP,
	A2DP_CMD_QUIT,
} a2dp_command_t;

struct bluetooth_data {
	unsigned int link_mtu;			/* MTU for transport channel */
	struct pollfd stream;			/* Audio stream filedescriptor */
	struct pollfd server;			/* Audio daemon filedescriptor */
	a2dp_state_t state;				/* Current A2DP state */
	a2dp_command_t command;			/* Current command for a2dp_thread */
	pthread_t thread;
	pthread_mutex_t mutex;
	int started;
	pthread_cond_t thread_start;
	pthread_cond_t thread_wait;
	pthread_cond_t client_wait;

	sbc_capabilities_t sbc_capabilities;
	sbc_t sbc;				/* Codec data */
	int	frame_duration;		/* length of an SBC/MP3 frame in microseconds */
	int codesize;				/* SBC codesize */
	int samples;				/* Number of encoded samples */
	uint8_t buffer[BUFFER_SIZE];		/* Codec transfer buffer */
	int count;				/* Codec transfer buffer counter */

	int codec_configured;
	int codec_supported_remote;

#ifdef ENABLE_MP3_ITTIAM_CODEC
	mpeg_capabilities_t mpeg_capabilities;
	uint8_t *pending_buffer_mp3;
	int filledLen_pending_buffer_mp3;
	int size_pending_buffer_mp3;
	int  sbc_fallback;

	void *p_mp3_enc_handle;
	ia_mp3_enc_params_t mp3_enc_params;

#ifdef ADD_RTP_PAYLOAD_HEADER
	void *mpeg12pl_handle;
	mpeg12pl_attr_t     mpeg12pl_attr;
	mpeg12pl_cb_funcs_t mpeg12pl_cb_funcs;
	mpeg12pl_packet_t   mpeg12pl_packet;
	int mpeg12pl_audio_header_size;
#endif //ADD_RTP_PAYLOAD_HEADER
#endif //ENABLE_MP3_ITTIAM_CODEC
	int nsamples;				/* Cumulative number of codec samples */
	uint16_t seq_num;			/* Cumulative packet sequence */
	int frame_count;			/* Current frames in buffer*/

	char	address[20];
	int	rate;
	int	channels;

	/* used for pacing our writes to the output socket */
	uint64_t	next_write;

#ifdef ENABLE_CSR_APTX_CODEC
	btaptx_capabilities_t btaptx_capabilities;
	btaptx_t btaptx;
	void    *aptxCodec;
#endif //ENABLE_CSR_APTX_CODEC
	int codec_priority;

};

uint64_t prevA2dpWriteTime, endTime;

static uint64_t get_microseconds()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1000000UL + now.tv_nsec / 1000UL);
}

#ifdef ENABLE_TIMING
static void print_time(const char* message, uint64_t then, uint64_t now)
{
	DBG("%s: %lld us", message, now - then);
}
#endif

static int audioservice_send(struct bluetooth_data *data, const bt_audio_msg_header_t *msg);
static int audioservice_expect(struct bluetooth_data *data, bt_audio_msg_header_t *outmsg,
				int expected_type);
#ifdef ENABLE_MP3_ITTIAM_CODEC
static int bluetooth_a2dp_mp3_hw_params(struct bluetooth_data *data);
#endif //ENABLE_MP3_ITTIAM_CODEC

#ifdef ENABLE_CSR_APTX_CODEC
static int bluetooth_a2dp_btaptx_hw_params(struct bluetooth_data *data);
#endif //ENABLE_CSR_APTX_CODEC

static int bluetooth_a2dp_hw_params(struct bluetooth_data *data);
static void set_state(struct bluetooth_data *data, a2dp_state_t state);


static void bluetooth_close(struct bluetooth_data *data)
{
	DBG("bluetooth_close");
	if (data->server.fd >= 0) {
		bt_audio_service_close(data->server.fd);
		data->server.fd = -1;
	}

	if (data->stream.fd >= 0) {
		close(data->stream.fd);
		data->stream.fd = -1;
	}

	data->state = A2DP_STATE_NONE;
}

static int l2cap_set_flushable(int fd, int flushable)
{
	int flags;
	socklen_t len;

	len = sizeof(flags);
	if (getsockopt(fd, SOL_L2CAP, L2CAP_LM, &flags, &len) < 0)
		return -errno;

	if (flushable) {
		if (flags & L2CAP_LM_FLUSHABLE)
			return 0;
		flags |= L2CAP_LM_FLUSHABLE;
	} else {
		if (!(flags & L2CAP_LM_FLUSHABLE))
			return 0;
		flags &= ~L2CAP_LM_FLUSHABLE;
	}

	if (setsockopt(fd, SOL_L2CAP, L2CAP_LM, &flags, sizeof(flags)) < 0)
		return -errno;

	return 0;
}

static int bluetooth_start(struct bluetooth_data *data)
{
	char c = 'w';
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_start_stream_req *start_req = (void*) buf;
	struct bt_start_stream_rsp *start_rsp = (void*) buf;
	struct bt_new_stream_ind *streamfd_ind = (void*) buf;
	int opt_name, err, bytes;

	DBG("bluetooth_start");
	data->state = A2DP_STATE_STARTING;
	/* send start */
	memset(start_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	start_req->h.type = BT_REQUEST;
	start_req->h.name = BT_START_STREAM;
	start_req->h.length = sizeof(*start_req);


	err = audioservice_send(data, &start_req->h);
	if (err < 0)
		goto error;

	start_rsp->h.length = sizeof(*start_rsp);
	err = audioservice_expect(data, &start_rsp->h, BT_START_STREAM);
	if (err < 0)
		goto error;

	streamfd_ind->h.length = sizeof(*streamfd_ind);
	err = audioservice_expect(data, &streamfd_ind->h, BT_NEW_STREAM);
	if (err < 0)
		goto error;

	data->stream.fd = bt_audio_service_get_data_fd(data->server.fd);
	if (data->stream.fd < 0) {
		ERR("bt_audio_service_get_data_fd failed, errno: %d", errno);
		err = -errno;
		goto error;
	}
	l2cap_set_flushable(data->stream.fd, 1);
	data->stream.events = POLLOUT;

	/* set our socket buffer to the size of PACKET_BUFFER_COUNT packets */
	bytes = data->link_mtu * PACKET_BUFFER_COUNT;
	setsockopt(data->stream.fd, SOL_SOCKET, SO_SNDBUF, &bytes,
			sizeof(bytes));

	data->count = sizeof(struct rtp_header) + sizeof(struct rtp_payload);
	data->frame_count = 0;
	data->samples = 0;
	data->nsamples = 0;
	data->seq_num = 0;
	data->frame_count = 0;
	data->next_write = 0;

	set_state(data, A2DP_STATE_STARTED);
	return 0;

error:
	/* close bluetooth connection to force reinit and reconfiguration */
	if (data->state == A2DP_STATE_STARTING)
		bluetooth_close(data);
	return err;
}

static int bluetooth_stop(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_stop_stream_req *stop_req = (void*) buf;
	struct bt_stop_stream_rsp *stop_rsp = (void*) buf;
	int err;

	DBG("bluetooth_stop");

	data->state = A2DP_STATE_STOPPING;
	l2cap_set_flushable(data->stream.fd, 0);
	if (data->stream.fd >= 0) {
		close(data->stream.fd);
		data->stream.fd = -1;
	}

	/* send stop request */
	memset(stop_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	stop_req->h.type = BT_REQUEST;
	stop_req->h.name = BT_STOP_STREAM;
	stop_req->h.length = sizeof(*stop_req);

	err = audioservice_send(data, &stop_req->h);
	if (err < 0)
		goto error;

	stop_rsp->h.length = sizeof(*stop_rsp);
	err = audioservice_expect(data, &stop_rsp->h, BT_STOP_STREAM);
	if (err < 0)
		goto error;

error:
	if (data->state == A2DP_STATE_STOPPING)
		set_state(data, A2DP_STATE_CONFIGURED);
	return err;
}

static uint8_t default_bitpool(uint8_t freq, uint8_t mode)
{
	switch (freq) {
	case BT_SBC_SAMPLING_FREQ_16000:
	case BT_SBC_SAMPLING_FREQ_32000:
		return 53;
	case BT_SBC_SAMPLING_FREQ_44100:
		switch (mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			return 31;
		case BT_A2DP_CHANNEL_MODE_STEREO:
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			/* max bit pool considered for this mode
			   is 64 in a2dp profile and the same
			   is mapped here as well.
			*/
			return 53;
		default:
			ERR("Invalid channel mode %u", mode);
			return 53;
		}
	case BT_SBC_SAMPLING_FREQ_48000:
		switch (mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			return 29;
		case BT_A2DP_CHANNEL_MODE_STEREO:
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			return 51;
		default:
			ERR("Invalid channel mode %u", mode);
			return 51;
		}
	default:
		ERR("Invalid sampling freq %u", freq);
		return 53;
	}
}

static int bluetooth_a2dp_init(struct bluetooth_data *data)
{
	sbc_capabilities_t *cap = &data->sbc_capabilities;
	unsigned int max_bitpool, min_bitpool;
	int dir;

#ifdef ENABLE_MP3_ITTIAM_CODEC
	mpeg_capabilities_t *mp3_cap = &data->mpeg_capabilities;

	if (data->codec_configured == A2DP_CODEC_MPEG12) {
		DBG(" Initializing bluetooth_a2dp for mp3");
		switch (data->rate) {
		case 48000:
			mp3_cap->frequency = BT_MPEG_SAMPLING_FREQ_48000;
			break;
		case 44100:
			mp3_cap->frequency = BT_MPEG_SAMPLING_FREQ_44100;
			break;
		case 32000:
			mp3_cap->frequency = BT_MPEG_SAMPLING_FREQ_32000;
			break;
		case 16000:
			mp3_cap->frequency = BT_MPEG_SAMPLING_FREQ_16000;
			break;
		default:
			ERR("Rate %d not supported", data->rate);
			return -1;
		}

		mp3_cap->bitrate = BT_MPEG_BIT_RATE_VBR;
		mp3_cap->layer = BT_MPEG_LAYER_3;
		mp3_cap->crc = 0;

		if (data->channels == 2) {
			if (mp3_cap->channel_mode &
				BT_A2DP_CHANNEL_MODE_JOINT_STEREO)
				mp3_cap->channel_mode = BT_A2DP_CHANNEL_MODE_JOINT_STEREO;
			else if (mp3_cap->channel_mode &
				BT_A2DP_CHANNEL_MODE_STEREO)
				mp3_cap->channel_mode = BT_A2DP_CHANNEL_MODE_STEREO;
			else if (mp3_cap->channel_mode &
				BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL)
				mp3_cap->channel_mode = BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL;
		} else {
			if (mp3_cap->channel_mode &
				BT_A2DP_CHANNEL_MODE_MONO)
				mp3_cap->channel_mode = BT_A2DP_CHANNEL_MODE_MONO;
		}

	} // End MP3 Cap.
	else {
#endif //ENABLE_MP3_ITTIAM_CODEC
	DBG(" Initializing bluetooth_a2dp for SBC");
	switch (data->rate) {
	case 48000:
		cap->frequency = BT_SBC_SAMPLING_FREQ_48000;
		break;
	case 44100:
		cap->frequency = BT_SBC_SAMPLING_FREQ_44100;
		break;
	case 32000:
		cap->frequency = BT_SBC_SAMPLING_FREQ_32000;
		break;
	case 16000:
		cap->frequency = BT_SBC_SAMPLING_FREQ_16000;
		break;
	default:
		ERR("Rate %d not supported", data->rate);
		return -1;
	}

	if (data->channels == 2) {
		if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_JOINT_STEREO)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_JOINT_STEREO;
		else if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_STEREO)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_STEREO;
		else if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL;
	} else {
		if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_MONO)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_MONO;
	}

	if (!cap->channel_mode) {
		ERR("No supported channel modes");
		return -1;
	}

	if (cap->block_length & BT_A2DP_BLOCK_LENGTH_16)
		cap->block_length = BT_A2DP_BLOCK_LENGTH_16;
	else if (cap->block_length & BT_A2DP_BLOCK_LENGTH_12)
		cap->block_length = BT_A2DP_BLOCK_LENGTH_12;
	else if (cap->block_length & BT_A2DP_BLOCK_LENGTH_8)
		cap->block_length = BT_A2DP_BLOCK_LENGTH_8;
	else if (cap->block_length & BT_A2DP_BLOCK_LENGTH_4)
		cap->block_length = BT_A2DP_BLOCK_LENGTH_4;
	else {
		ERR("No supported block lengths");
		return -1;
	}

	if (cap->subbands & BT_A2DP_SUBBANDS_8)
		cap->subbands = BT_A2DP_SUBBANDS_8;
	else if (cap->subbands & BT_A2DP_SUBBANDS_4)
		cap->subbands = BT_A2DP_SUBBANDS_4;
	else {
		ERR("No supported subbands");
		return -1;
	}

	if (cap->allocation_method & BT_A2DP_ALLOCATION_LOUDNESS)
		cap->allocation_method = BT_A2DP_ALLOCATION_LOUDNESS;
	else if (cap->allocation_method & BT_A2DP_ALLOCATION_SNR)
		cap->allocation_method = BT_A2DP_ALLOCATION_SNR;

		min_bitpool = MAX(MIN_BITPOOL, cap->min_bitpool);
		max_bitpool = MIN(default_bitpool(cap->frequency,
					cap->channel_mode),
					cap->max_bitpool);

	cap->min_bitpool = min_bitpool;
	cap->max_bitpool = max_bitpool;

#ifdef ENABLE_MP3_ITTIAM_CODEC
	}
#endif //ENABLE_MP3_ITTIAM_CODEC
	return 0;
}

#ifdef ENABLE_MP3_ITTIAM_CODEC
static int bluetooth_a2dp_mp3_setup(struct bluetooth_data *data)
{
	int err_code = 0, bitrate = 192000;
	int frequency = 44100, mode = MPEG_CHANNEL_MODE_JOINT_STEREO;
	ia_mp3_enc_params_t* pmp3_enc_params = &data->mp3_enc_params;

	mpeg_capabilities_t active_capabilities = data->mpeg_capabilities;
	DBG("We are in bluetooth_a2dp_mp3_setup\n");

	memset(pmp3_enc_params, 0, sizeof(ia_mp3_enc_params_t));

	if (active_capabilities.frequency & BT_MPEG_SAMPLING_FREQ_16000)
		frequency = 16000;

	if (active_capabilities.frequency & BT_MPEG_SAMPLING_FREQ_22050)
		frequency = 22050;

	if (active_capabilities.frequency & BT_MPEG_SAMPLING_FREQ_24000)
		frequency = 24000;

	if (active_capabilities.frequency & BT_MPEG_SAMPLING_FREQ_32000)
		frequency = 32000;

	if (active_capabilities.frequency & BT_MPEG_SAMPLING_FREQ_44100)
		frequency = 44100;

	if (active_capabilities.frequency & BT_MPEG_SAMPLING_FREQ_48000)
		frequency = 48000;

	if (active_capabilities.channel_mode & MPEG_CHANNEL_MODE_MONO)
		mode = MPEG_CHANNEL_MODE_MONO;

	if (active_capabilities.channel_mode & MPEG_CHANNEL_MODE_DUAL_CHANNEL)
		mode = MPEG_CHANNEL_MODE_DUAL_CHANNEL;

	if (active_capabilities.channel_mode & MPEG_CHANNEL_MODE_STEREO)
		mode = MPEG_CHANNEL_MODE_STEREO;

	if (active_capabilities.channel_mode & MPEG_CHANNEL_MODE_JOINT_STEREO)
		mode = MPEG_CHANNEL_MODE_JOINT_STEREO;

	bitrate = active_capabilities.bitrate;
	data->codesize = 1152  * sizeof(short) * 1;//sbc_get_codesize(&data->sbc);
	if ( mode != MPEG_CHANNEL_MODE_MONO)
		data->codesize *= 2 ; //for two channels

	data->frame_duration = 1152/frequency; //in seconds
	data->frame_duration = (1152 * 1000000)/ frequency; //in micro-seconds
	DBG("MP3 frame_duration: %d us", data->frame_duration);

	pmp3_enc_params->size = sizeof(ia_mp3_enc_params_t);
	pmp3_enc_params->sampleRate = frequency;
	pmp3_enc_params->channelMode = mode;
	pmp3_enc_params->bitRate = 192000; //We can chage bitrates here
	pmp3_enc_params->packet = 0;

	data->p_mp3_enc_handle = NULL;
	data->pending_buffer_mp3 = (uint8_t *)malloc(MP3_BUFFER_SIZE);

	if( data->pending_buffer_mp3 == NULL) {
		ERR("Could not allocate internal buffer \n");
		return -EINVAL;
	} else {
		memset(data->pending_buffer_mp3 , 0, MP3_BUFFER_SIZE );
		data->filledLen_pending_buffer_mp3 = 0;
		data->size_pending_buffer_mp3 = MP3_BUFFER_SIZE;
	}

	DBG("ia_mp3_enc_ocp_init Internal Input Buffer =0x%x \n", data->pending_buffer_mp3);
	DBG("ia_mp3_enc_ocp_init bitRate=%d \n", pmp3_enc_params->bitRate);
	DBG("ia_mp3_enc_ocp_init sampleRate=%d \n", pmp3_enc_params->sampleRate);
	DBG("ia_mp3_enc_ocp_init size=%d \n", pmp3_enc_params->size);
	DBG("ia_mp3_enc_ocp_init packet=%d \n", pmp3_enc_params->packet);
	DBG("ia_mp3_enc_ocp_init mode=%d \n", pmp3_enc_params->channelMode);

	err_code = ia_mp3_enc_ocp_init(&(data->p_mp3_enc_handle), pmp3_enc_params);

	if (err_code != 0) {
		ERR("Codec init failed with error:%#x", err_code);
		return -EINVAL;
	} else {
		DBG("MP3 Encoder Initialized successfully ");
	}
#ifdef ADD_RTP_PAYLOAD_HEADER
	err_code = ia_mpeg12pl_rtp_init(&(data->mpeg12pl_handle),
						&(data->mpeg12pl_attr),
						&(data->mpeg12pl_cb_funcs),
						&(data->mpeg12pl_packet));

	if (err_code != 0) {
		ERR("mpeg12pl init failed with error:%#x", err_code);
		return -EINVAL;
	} else {
		DBG("RTP_PAYLOAD_HEADER Initialized successfully ");
	}
	mpeg12pl_get_header_offset(data->mpeg12pl_handle,
					&data->mpeg12pl_audio_header_size);
#endif //ADD_RTP_PAYLOAD_HEADER

	return 0;
}
#endif //ENABLE_MP3_ITTIAM_CODEC

static void bluetooth_a2dp_setup(struct bluetooth_data *data)
{
	sbc_capabilities_t active_capabilities = data->sbc_capabilities;

	sbc_reinit(&data->sbc, 0);

	if (active_capabilities.frequency & BT_SBC_SAMPLING_FREQ_16000)
		data->sbc.frequency = SBC_FREQ_16000;

	if (active_capabilities.frequency & BT_SBC_SAMPLING_FREQ_32000)
		data->sbc.frequency = SBC_FREQ_32000;

	if (active_capabilities.frequency & BT_SBC_SAMPLING_FREQ_44100)
		data->sbc.frequency = SBC_FREQ_44100;

	if (active_capabilities.frequency & BT_SBC_SAMPLING_FREQ_48000)
		data->sbc.frequency = SBC_FREQ_48000;

	if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_MONO)
		data->sbc.mode = SBC_MODE_MONO;

	if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL)
		data->sbc.mode = SBC_MODE_DUAL_CHANNEL;

	if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_STEREO)
		data->sbc.mode = SBC_MODE_STEREO;

	if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_JOINT_STEREO)
		data->sbc.mode = SBC_MODE_JOINT_STEREO;

	data->sbc.allocation = active_capabilities.allocation_method
				== BT_A2DP_ALLOCATION_SNR ? SBC_AM_SNR
				: SBC_AM_LOUDNESS;

	switch (active_capabilities.subbands) {
	case BT_A2DP_SUBBANDS_4:
		data->sbc.subbands = SBC_SB_4;
		break;
	case BT_A2DP_SUBBANDS_8:
		data->sbc.subbands = SBC_SB_8;
		break;
	}

	switch (active_capabilities.block_length) {
	case BT_A2DP_BLOCK_LENGTH_4:
		data->sbc.blocks = SBC_BLK_4;
		break;
	case BT_A2DP_BLOCK_LENGTH_8:
		data->sbc.blocks = SBC_BLK_8;
		break;
	case BT_A2DP_BLOCK_LENGTH_12:
		data->sbc.blocks = SBC_BLK_12;
		break;
	case BT_A2DP_BLOCK_LENGTH_16:
		data->sbc.blocks = SBC_BLK_16;
		break;
	}

	data->sbc.bitpool = active_capabilities.max_bitpool;
	data->codesize = sbc_get_codesize(&data->sbc);
	data->frame_duration = sbc_get_frame_duration(&data->sbc);
	DBG("frame_duration: %d us", data->frame_duration);
}

#ifdef ENABLE_MP3_ITTIAM_CODEC
static int bluetooth_a2dp_mp3_hw_params(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_open_req *open_req = (void *) buf;
	struct bt_open_rsp *open_rsp = (void *) buf;
	struct bt_set_configuration_req *setconf_req = (void*) buf;
	struct bt_set_configuration_rsp *setconf_rsp = (void*) buf;
	int err=0;

	DBG("We are in bluetooth_a2dp_mp3_hw_params()\n");

	memset(open_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	open_req->h.type = BT_REQUEST;
	open_req->h.name = BT_OPEN;
	open_req->h.length = sizeof(*open_req);
	strncpy(open_req->destination, data->address, 18);

	if (data->codec_configured == A2DP_CODEC_MPEG12) {
		open_req->seid = data->mpeg_capabilities.capability.seid;
	}
	DBG("bluetooth_a2dp_mp3_hw_params : Codec %d SEID %d",
		data->codec_configured, open_req->seid);

	if (!open_req->seid) {
		ERR("open_req->seid is NULL return -EINVAL for bluetooth_a2dp_mp3_hw_params");
		return -EINVAL;
	}

	open_req->lock = BT_WRITE_LOCK;

	DBG("open_req->h.type : BT_REQUEST=%s\n", bt_audio_strname(open_req->h.type));
	DBG("open_req->h.name : BT_OPEN=%s\n", bt_audio_strname(open_req->h.name));
	DBG("open_req->h.length : sizeof(*open_req)=%u\n",open_req->h.length);
	DBG("strncpy(open_req->destination, data->address,18=%s\n",open_req->destination);
	DBG("open_req->seid : data->mpeg_capabilities.capability.seid=%u\n", open_req->seid);
	DBG("open_req->lock : BT_WRITE_LOCK=%u\n", open_req->lock);

	err = audioservice_send(data, &open_req->h);
	if (err < 0) {
		ERR("Error audioservice_send : %s(%d)",
				strerror(errno), errno);
		return err;
	}

	open_rsp->h.length = sizeof(*open_rsp);
	err = audioservice_expect(data, &open_rsp->h, BT_OPEN);
	if (err < 0) {
		ERR("Error audioservice_expect: %s(%d)",
				strerror(errno), errno);
		return err;
	}

	DBG("Calling bluetooth_mp3_init from hw param\n");
	err = bluetooth_a2dp_init(data);
	if (err < 0){
		ERR("Error bluetooth_a2dp_init: %s(%d)",
				strerror(errno), errno);
		return err;
	}

	memset(setconf_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	setconf_req->h.type = BT_REQUEST;
	setconf_req->h.name = BT_SET_CONFIGURATION;
	setconf_req->h.length = sizeof(*setconf_req);

	memcpy(&setconf_req->codec, &data->mpeg_capabilities,
						sizeof(data->mpeg_capabilities));

	setconf_req->codec.transport = BT_CAPABILITIES_TRANSPORT_A2DP;
	setconf_req->codec.length = sizeof(data->mpeg_capabilities);
	setconf_req->h.length += setconf_req->codec.length - sizeof(setconf_req->codec);

	DBG("setconf_req->h.type:BT_REQUEST=%s\n", bt_audio_strname(setconf_req->h.type));
	DBG("setconf_req->h.name:BT_SET_CONFIGURATION=%s", bt_audio_strname(setconf_req->h.name));
	DBG("bluetooth_a2dp_mp3_hw_params sending configuration:\n");

	switch (data->mpeg_capabilities.channel_mode) {
	case BT_A2DP_CHANNEL_MODE_MONO:
		DBG("channel_mode: MONO");
		break;
	case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
		DBG("channel_mode: DUAL CHANNEL");
		break;
	case BT_A2DP_CHANNEL_MODE_STEREO:
		DBG("channel_mode: STEREO");
		break;
	case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
		DBG("channel_mode: JOINT STEREO");
		break;
	default:
		DBG("channel_mode: UNKNOWN (%d)",
			data->mpeg_capabilities.channel_mode);
	}

	switch (data->mpeg_capabilities.frequency) {
	case BT_MPEG_SAMPLING_FREQ_16000:
		DBG("frequency: 16000");
		break;
	case BT_MPEG_SAMPLING_FREQ_22050:
		DBG("frequency: 22050");
		break;
	case BT_MPEG_SAMPLING_FREQ_24000:
		DBG("frequency: 24000");
		break;
	case BT_MPEG_SAMPLING_FREQ_32000:
		DBG("frequency: 32000");
		break;
	case BT_MPEG_SAMPLING_FREQ_44100:
		DBG("frequency: 44100");
		break;
	case BT_MPEG_SAMPLING_FREQ_48000:
		DBG("frequency: 48000");
		break;
	default:
		DBG("frequency: UNKNOWN (%d)",
			data->mpeg_capabilities.frequency);
	}

	switch (data->mpeg_capabilities.layer) {
	case MPEG_LAYER_MP1:
		DBG("layer: MPEG_LAYER_MP1");
		break;
	case MPEG_LAYER_MP2:
		DBG("layer: MPEG_LAYER_MP2");
		break;
	case MPEG_LAYER_MP3:
		DBG("layer: MPEG_LAYER_MP3");
		break;
	default:
		DBG("layer: UNKNOWN (%d)",
			data->mpeg_capabilities.layer);
	}

	switch (data->mpeg_capabilities.bitrate) {
	case MP3_BITRATE_32:
		DBG("\tbitrate: MP3_BITRATE_32\n");
		break;
	case MP3_BITRATE_40:
		DBG("\tbitrate: MP3_BITRATE_40\n");
		break;
	case MP3_BITRATE_48:
		DBG("\tbitrate: MP3_BITRATE_48\n");
		break;
	case MP3_BITRATE_56:
		DBG("\tbitrate: MP3_BITRATE_56\n");
		break;
	case MP3_BITRATE_64:
		DBG("\tbitrate: MP3_BITRATE_64\n");
		break;
	case MP3_BITRATE_80:
		DBG("\tbitrate: MP3_BITRATE_80\n");
		break;
	case MP3_BITRATE_96:
		DBG("\tbitrate: MP3_BITRATE_96\n");
		break;
	case MP3_BITRATE_112:
		DBG("\tbitrate: MP3_BITRATE_112\n");
		break;
	case MP3_BITRATE_128:
		DBG("\tbitrate: MP3_BITRATE_128\n");
		break;
	case MP3_BITRATE_160:
		DBG("\tbitrate: MP3_BITRATE_160\n");
		break;
	case MP3_BITRATE_192:
		DBG("\tbitrate: MP3_BITRATE_192\n");
		break;
	case MP3_BITRATE_224:
		DBG("\tbitrate: MP3_BITRATE_224\n");
		break;
	case MP3_BITRATE_256:
		DBG("\tbitrate: MP3_BITRATE_256\n");
		break;
	case MP3_BITRATE_320:
		DBG("\tbitrate: MP3_BITRATE_320\n");
		break;
	default:
		DBG("\tbitrate: UNKNOWN (%x)\n",
				data->mpeg_capabilities.bitrate);
	}

	err = audioservice_send(data, &setconf_req->h);
	if (err < 0)
		return err;

	err = audioservice_expect(data, &setconf_rsp->h, BT_SET_CONFIGURATION);
	if (err < 0)
		return err;

	data->link_mtu = setconf_rsp->link_mtu;
	DBG("MTU: %d", data->link_mtu);

	/* Setup MP3 encoder now we agree on parameters */
	DBG("Setup MP3 encoder now we agree on parameters calling bluetooth_a2dp_mp3_setup\n");
	err = bluetooth_a2dp_mp3_setup(data);
	if (err < 0)
		return err;

	DBG("end of bluetooth_a2dp_mp3_hw_btaptx_params()\n");

	return 0;
}
#endif //ENABLE_MP3_ITTIAM_CODEC

static int bluetooth_a2dp_hw_params(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_open_req *open_req = (void *) buf;
	struct bt_open_rsp *open_rsp = (void *) buf;
	struct bt_set_configuration_req *setconf_req = (void*) buf;
	struct bt_set_configuration_rsp *setconf_rsp = (void*) buf;
	int err;

	memset(open_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	open_req->h.type = BT_REQUEST;
	open_req->h.name = BT_OPEN;
	open_req->h.length = sizeof(*open_req);
	strncpy(open_req->destination, data->address, 18);
	open_req->seid = data->sbc_capabilities.capability.seid;
	open_req->lock = BT_WRITE_LOCK;

	err = audioservice_send(data, &open_req->h);
	if (err < 0)
		return err;

	open_rsp->h.length = sizeof(*open_rsp);
	err = audioservice_expect(data, &open_rsp->h, BT_OPEN);
	if (err < 0)
		return err;

	err = bluetooth_a2dp_init(data);
	if (err < 0)
		return err;


	memset(setconf_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	setconf_req->h.type = BT_REQUEST;
	setconf_req->h.name = BT_SET_CONFIGURATION;
	setconf_req->h.length = sizeof(*setconf_req);
	memcpy(&setconf_req->codec, &data->sbc_capabilities,
						sizeof(data->sbc_capabilities));

	setconf_req->codec.transport = BT_CAPABILITIES_TRANSPORT_A2DP;
	setconf_req->codec.length = sizeof(data->sbc_capabilities);
	setconf_req->h.length += setconf_req->codec.length - sizeof(setconf_req->codec);

	DBG("bluetooth_a2dp_hw_params sending configuration:\n");
	switch (data->sbc_capabilities.channel_mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
			DBG("\tchannel_mode: MONO\n");
			break;
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			DBG("\tchannel_mode: DUAL CHANNEL\n");
			break;
		case BT_A2DP_CHANNEL_MODE_STEREO:
			DBG("\tchannel_mode: STEREO\n");
			break;
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			DBG("\tchannel_mode: JOINT STEREO\n");
			break;
		default:
			DBG("\tchannel_mode: UNKNOWN (%d)\n",
				data->sbc_capabilities.channel_mode);
	}
	switch (data->sbc_capabilities.frequency) {
		case BT_SBC_SAMPLING_FREQ_16000:
			DBG("\tfrequency: 16000\n");
			break;
		case BT_SBC_SAMPLING_FREQ_32000:
			DBG("\tfrequency: 32000\n");
			break;
		case BT_SBC_SAMPLING_FREQ_44100:
			DBG("\tfrequency: 44100\n");
			break;
		case BT_SBC_SAMPLING_FREQ_48000:
			DBG("\tfrequency: 48000\n");
			break;
		default:
			DBG("\tfrequency: UNKNOWN (%d)\n",
				data->sbc_capabilities.frequency);
	}
	switch (data->sbc_capabilities.allocation_method) {
		case BT_A2DP_ALLOCATION_SNR:
			DBG("\tallocation_method: SNR\n");
			break;
		case BT_A2DP_ALLOCATION_LOUDNESS:
			DBG("\tallocation_method: LOUDNESS\n");
			break;
		default:
			DBG("\tallocation_method: UNKNOWN (%d)\n",
				data->sbc_capabilities.allocation_method);
	}
	switch (data->sbc_capabilities.subbands) {
		case BT_A2DP_SUBBANDS_4:
			DBG("\tsubbands: 4\n");
			break;
		case BT_A2DP_SUBBANDS_8:
			DBG("\tsubbands: 8\n");
			break;
		default:
			DBG("\tsubbands: UNKNOWN (%d)\n",
				data->sbc_capabilities.subbands);
	}
	switch (data->sbc_capabilities.block_length) {
		case BT_A2DP_BLOCK_LENGTH_4:
			DBG("\tblock_length: 4\n");
			break;
		case BT_A2DP_BLOCK_LENGTH_8:
			DBG("\tblock_length: 8\n");
			break;
		case BT_A2DP_BLOCK_LENGTH_12:
			DBG("\tblock_length: 12\n");
			break;
		case BT_A2DP_BLOCK_LENGTH_16:
			DBG("\tblock_length: 16\n");
			break;
		default:
			DBG("\tblock_length: UNKNOWN (%d)\n",
				data->sbc_capabilities.block_length);
	}
	DBG("\tmin_bitpool: %d\n", data->sbc_capabilities.min_bitpool);
	DBG("\tmax_bitpool: %d\n", data->sbc_capabilities.max_bitpool);

	err = audioservice_send(data, &setconf_req->h);
	if (err < 0)
		return err;

	err = audioservice_expect(data, &setconf_rsp->h, BT_SET_CONFIGURATION);
	if (err < 0)
		return err;

	data->link_mtu = setconf_rsp->link_mtu;
	DBG("MTU: %d", data->link_mtu);

	/* Setup SBC encoder now we agree on parameters */
	bluetooth_a2dp_setup(data);

	DBG("\tallocation=%u\n\tsubbands=%u\n\tblocks=%u\n\tbitpool=%u\n",
		data->sbc.allocation, data->sbc.subbands, data->sbc.blocks,
		data->sbc.bitpool);

	return 0;
}

static int avdtp_write(struct bluetooth_data *data)
{
	int ret = 0;
	struct rtp_header *header;
	struct rtp_payload *payload;

	uint64_t now;
	long duration = data->frame_duration * data->frame_count;
	uint64_t begin;
#ifdef ENABLE_TIMING
	uint64_t end, begin2, end2, begin3, end3;
#endif
	begin = get_microseconds();

	header = (struct rtp_header *)data->buffer;
	payload = (struct rtp_payload *)(data->buffer + sizeof(*header));

	memset(data->buffer, 0, sizeof(*header) + sizeof(*payload));

	header->v = 2;
	header->sequence_number = htons(data->seq_num);
	header->ssrc = htonl(1);

	data->stream.revents = 0;

#ifdef ENABLE_MP3_ITTIAM_CODEC
	if (data->codec_configured == A2DP_CODEC_MPEG12) {
		VDBG("avdtp_write: codec_configured A2DP_CODEC_MPEG12" );
		header->timestamp = htonl((data->frame_duration) *
						data->nsamples);
		header->pt = 0x0e;
	} else {
#endif // ENABLE_MP3_ITTIAM_CODEC
		VDBG("avdtp_write: codec_configured SBC" );
		payload->frame_count = data->frame_count;
		header->timestamp = htonl(data->nsamples);
		header->pt = 1;
#ifdef ENABLE_MP3_ITTIAM_CODEC
	}
#endif // ENABLE_MP3_ITTIAM_CODEC


#ifdef ENABLE_TIMING
	begin2 = get_microseconds();
#endif
	ret = poll(&data->stream, 1, POLL_TIMEOUT);
#ifdef ENABLE_TIMING
	end2 = get_microseconds();
	print_time("poll", begin2, end2);
#endif
	if (ret == 1 && data->stream.revents == POLLOUT) {
		long ahead = 0;
		now = get_microseconds();

		if (data->next_write) {
			ahead = data->next_write - now;
#ifdef ENABLE_TIMING
			DBG("duration: %ld, ahead: %ld", duration, ahead);
#endif
			if (ahead > 0) {
				/* too fast, need to throttle */
				usleep(ahead);
			}
		} else {
			data->next_write = now;
		}
		if (ahead <= -CATCH_UP_TIMEOUT * 1000) {
			/* fallen too far behind, don't try to catch up */
			VDBG("ahead < %d, reseting next_write timestamp", -CATCH_UP_TIMEOUT * 1000);
			data->next_write = 0;
		} else {
			data->next_write += duration;
		}

#ifdef ENABLE_TIMING
		begin3 = get_microseconds();
#endif
		ret = send(data->stream.fd, data->buffer, data->count, MSG_NOSIGNAL);
#ifdef ENABLE_TIMING
		end3 = get_microseconds();
		print_time("send", begin3, end3);
#endif
		if (ret < 0) {
			/* can happen during normal remote disconnect */
			VDBG("send() failed: %d (errno %s)", ret, strerror(errno));
			endTime = get_microseconds();
			prevA2dpWriteTime = endTime - begin;
		}
		if (ret == -EPIPE) {
			bluetooth_close(data);
		}
		ret = 0;
	} else {
		/* can happen during normal remote disconnect */
		VDBG("poll() failed: %d (revents = %d, errno %s) last successful a2dp_write %lld us",
				ret, data->stream.revents, strerror(errno), prevA2dpWriteTime);
		data->next_write = 0;
		/* Adding delay which is equal to last successfull a2dp write to make sure audio
		and video being played are in sync during HS disconnection. */
		usleep(prevA2dpWriteTime);
		ret = -1;
	}

#ifdef ENABLE_TIMING
	end = get_microseconds();
	DBG("avdtp_write poll %ll dus send %ll dus total %ll dus size %d",
		end2-begin2, end3-begin3, end-begin, data->count);

#endif

	/* Reset buffer of data to send */
	data->count = sizeof(struct rtp_header) + sizeof(struct rtp_payload);
	data->frame_count = 0;
	data->samples = 0;
	data->seq_num++;

	return ret;
}

static int audioservice_send(struct bluetooth_data *data,
		const bt_audio_msg_header_t *msg)
{
	int err;
	uint16_t length;

	length = msg->length ? msg->length : BT_SUGGESTED_BUFFER_SIZE;

	DBG("sending %s - %s", bt_audio_strtype(msg->type),
				bt_audio_strname(msg->name));
	if (send(data->server.fd, msg, length,
			MSG_NOSIGNAL) > 0)
		err = 0;
	else {
		err = -errno;
		ERR("Error sending data to audio service: %s(%d)",
			strerror(errno), errno);
		if (err == -EPIPE)
			bluetooth_close(data);
	}

	return err;
}

static int audioservice_recv(struct bluetooth_data *data,
		bt_audio_msg_header_t *inmsg)
{
	int err, ret;
	const char *type, *name;
	uint16_t length;

	length = inmsg->length ? inmsg->length : BT_SUGGESTED_BUFFER_SIZE;

	ret = recv(data->server.fd, inmsg, length, 0);
	if (ret < 0) {
		err = -errno;
		ERR("Error receiving IPC data from bluetoothd: %s (%d)",
						strerror(errno), errno);
		if (err == -EPIPE)
			bluetooth_close(data);
	} else if ((size_t) ret < sizeof(bt_audio_msg_header_t)) {
		ERR("Too short (%d bytes) IPC packet from bluetoothd", ret);
		err = -EINVAL;
	} else if (inmsg->type == BT_ERROR) {
		bt_audio_error_t *error = (bt_audio_error_t *)inmsg;
		ret = recv(data->server.fd, &error->posix_errno,
				sizeof(error->posix_errno), 0);
		if (ret < 0) {
			err = -errno;
			ERR("Error receiving error code for BT_ERROR: %s (%d)",
						strerror(errno), errno);
			if (err == -EPIPE)
				bluetooth_close(data);
		} else {
			ERR("%s failed : %s(%d)",
					bt_audio_strname(error->h.name),
					strerror(error->posix_errno),
					error->posix_errno);
			err = -error->posix_errno;
		}
	} else {
		type = bt_audio_strtype(inmsg->type);
		name = bt_audio_strname(inmsg->name);
		if (type && name) {
			DBG("Received %s - %s", type, name);
			err = 0;
		} else {
			err = -EINVAL;
			ERR("Bogus message type %d - name %d"
					" received from audio service",
					inmsg->type, inmsg->name);
		}

	}
	return err;
}

static int audioservice_expect(struct bluetooth_data *data,
		bt_audio_msg_header_t *rsp_hdr, int expected_name)
{
	int err = audioservice_recv(data, rsp_hdr);

	if (err != 0)
		return err;

	if (rsp_hdr->name != expected_name) {
		err = -EINVAL;
		ERR("Bogus message %s received while %s was expected",
				bt_audio_strname(rsp_hdr->name),
				bt_audio_strname(expected_name));
	}
	return err;

}

static int bluetooth_init(struct bluetooth_data *data)
{
	int sk, err;
	struct timeval tv = {.tv_sec = RECV_TIMEOUT};

	DBG("bluetooth_init");

	sk = bt_audio_service_open();
	if (sk < 0) {
		ERR("bt_audio_service_open failed\n");
		return -errno;
	}

	err = setsockopt(sk, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (err < 0) {
		ERR("bluetooth_init setsockopt(SO_RCVTIMEO) failed %d", err);
		return err;
	}

	data->server.fd = sk;
	data->server.events = POLLIN;
	data->state = A2DP_STATE_INITIALIZED;

	return 0;
}

static int bluetooth_parse_capabilities(struct bluetooth_data *data,
					struct bt_get_capabilities_rsp *rsp)
{
	int bytes_left = rsp->h.length - sizeof(*rsp);
	codec_capabilities_t *codec = (void *) rsp->data;
	data->codec_priority = 0;

	DBG("bluetooth_parse_capabilities codec %d %d ",codec->type,
							codec->transport);

	memset(&data->sbc_capabilities , 0 , sizeof(data->sbc_capabilities));
#ifdef ENABLE_MP3_ITTIAM_CODEC
	memset(&data->mpeg_capabilities , 0 , sizeof(data->mpeg_capabilities));
#endif //ENABLE_MP3_ITTIAM_CODEC
#ifdef ENABLE_CSR_APTX_CODEC
	memset(&data->btaptx_capabilities , 0 , sizeof(data->btaptx_capabilities));
#endif //ENABLE_CSR_APTX_CODEC


	if (codec->transport != BT_CAPABILITIES_TRANSPORT_A2DP)
		return -EINVAL;

	data->codec_priority = 0;
	data->codec_supported_remote = 0;

	while (bytes_left > 0) {
#ifdef ENABLE_CSR_APTX_CODEC
		if (codec->type == BT_A2DP_BTAPTX_SINK) {
			DBG("bluetooth_parse_capabilities codec APTX");
			if (bytes_left <= 0 ||
					codec->length != sizeof(data->btaptx_capabilities))
				return -EINVAL;
			data->codec_priority = 2;
			data->codec_supported_remote = data->codec_supported_remote | (1 << BT_A2DP_BTAPTX_SINK);
			DBG("codec supported remote = %d\n", data->codec_supported_remote);
			memcpy(&data->btaptx_capabilities, codec, codec->length);

			DBG("aptx codec ID0 = %u\n ",  data->btaptx_capabilities.codec_id0);
			DBG("aptx codec ID1 = %u\n ",  data->btaptx_capabilities.codec_id1);
			DBG("aptx Vendor ID0 = %u\n ", data->btaptx_capabilities.vender_id0);
			DBG("aptx Vendor ID1 = %u\n ", data->btaptx_capabilities.vender_id1);
			DBG("aptx Vendor ID2 = %u\n ", data->btaptx_capabilities.vender_id2);
			DBG("aptx Vendor ID3 = %u\n ", data->btaptx_capabilities.vender_id3);
			DBG("aptx channel mode = %u\n", data->btaptx_capabilities.channel_mode);
			DBG("aptx frequency = %u\n ",  data->btaptx_capabilities.frequency);

		} else
#endif //ENABLE_CSR_APTX_CODEC

#ifdef ENABLE_MP3_ITTIAM_CODEC
		//if (codec->type == BT_A2DP_MPEG12_SINK) {
		/*temp fix: music using codec MPEG12 with Motorola S10 playing with low
		volume. Need to further investigate. temporarily do not parse MPEG12 codec*/
		if(0) { 
			DBG("bluetooth_parse_capabilities codec MPEG12 ");
			if (bytes_left <= 0 ||
					codec->length != sizeof(data->mpeg_capabilities))
				return -EINVAL;
			memcpy(&data->mpeg_capabilities, codec, codec->length);

			mpeg_capabilities_t *mpeg_codec = (mpeg_capabilities_t *) codec->data;
			data->codec_priority=1;
			data->codec_supported_remote = data->codec_supported_remote |
								(1 << BT_A2DP_MPEG12_SINK);
			data->codec_configured = A2DP_CODEC_MPEG12;
			DBG(" data->codec_configured ",data->codec_configured);
			DBG("MPEG channel mode = %u ",data->mpeg_capabilities.channel_mode);
			DBG("MPEG frequency = %u ",data->mpeg_capabilities.frequency);
			DBG("MPEG bitrate = %u ", data->mpeg_capabilities.bitrate);
			DBG("MPEG layer= %u ", data->mpeg_capabilities.layer);
		} else
#endif //ENABLE_MP3_ITTIAM_CODEC
		if (codec->type == BT_A2DP_SBC_SINK) {
			DBG("bluetooth_parse_capabilities codec SBC");
			if (bytes_left <= 0 ||
					codec->length != sizeof(data->sbc_capabilities))
				return -EINVAL;

			memcpy(&data->sbc_capabilities, codec, codec->length);

			data->codec_priority=0;
			data->codec_supported_remote = data->codec_supported_remote |
								(1 << BT_A2DP_SBC_SINK);
			data->codec_configured = A2DP_CODEC_SBC;

			DBG("SBC channel mode = %u ",data->sbc_capabilities.channel_mode);
			DBG("SBC frequency = %u ",data->sbc_capabilities.frequency);
			DBG("SBC allocation_method= %u",data->sbc_capabilities.allocation_method);
			DBG("SBC subbands = %u ",data->sbc_capabilities.subbands);
			DBG("SBC block_length = %u ", data->sbc_capabilities.block_length);
			DBG("SBC min_bitpool = %u ", data->sbc_capabilities.min_bitpool);
			DBG("SBC max_bitpool = %u ", data->sbc_capabilities.max_bitpool);
		} else {
			DBG("Unknown codec type or not supported\n");
		}

		if (codec->length == 0) {
			ERR("bluetooth_parse_capabilities() invalid codec capabilities length");
			return -EINVAL;
		}
		bytes_left -= codec->length;
		codec = (codec_capabilities_t *)((char *)codec + codec->length);
	}
	return 0;
}

static int bluetooth_configure(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_get_capabilities_req *getcaps_req = (void*) buf;
	struct bt_get_capabilities_rsp *getcaps_rsp = (void*) buf;
	int err;

	DBG("bluetooth_configure");

	data->state = A2DP_STATE_CONFIGURING;
	memset(getcaps_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	getcaps_req->h.type = BT_REQUEST;
	getcaps_req->h.name = BT_GET_CAPABILITIES;

	getcaps_req->flags = 0;
	getcaps_req->flags |= BT_FLAG_AUTOCONNECT;
	strncpy(getcaps_req->destination, data->address, 18);
	getcaps_req->transport = BT_CAPABILITIES_TRANSPORT_A2DP;
	getcaps_req->h.length = sizeof(*getcaps_req);

	err = audioservice_send(data, &getcaps_req->h);
	if (err < 0) {
		ERR("audioservice_send failed for BT_GETCAPABILITIES_REQ\n");
		goto error;
	}

	getcaps_rsp->h.length = 0;
	err = audioservice_expect(data, &getcaps_rsp->h, BT_GET_CAPABILITIES);
	if (err < 0) {
		ERR("audioservice_expect failed for BT_GETCAPABILITIES_RSP\n");
		goto error;
	}

	err = bluetooth_parse_capabilities(data, getcaps_rsp);
	if (err < 0) {
		ERR("bluetooth_parse_capabilities failed err: %d", err);
		goto error;
	}

#ifdef ENABLE_CSR_APTX_CODEC
 	if ((data->codec_supported_remote & (1 << BT_A2DP_BTAPTX_SINK)) != 0) {
		DBG("btaptx codec found : calling \
			bluetooth_a2dp_btaptx_hw_params ");
		err = bluetooth_a2dp_btaptx_hw_params(data);
		if (err < 0) {
			ERR("bluetooth_a2dp_btaptx_hw_params failed err: %d", err);
			goto error;
		}
		data->codec_configured =  A2DP_CODEC_BTAPTX;
	}else
#endif //ENABLE_CSR_APTX_CODEC

#ifdef ENABLE_MP3_ITTIAM_CODEC
	if ((data->codec_supported_remote & (1 << BT_A2DP_MPEG12_SINK)) != 0) {
		DBG("mpeg12 codec found : calling bluetooth_a2dp_mp3_hw_params");
		DBG(" data->codec_priority = %d",data->codec_priority);
		data->codec_configured = A2DP_CODEC_MPEG12;
		err = bluetooth_a2dp_mp3_hw_params(data);
		if (err < 0) {
			ERR("bluetooth_a2dp_mp3_hw_params failed err: %d  %s", err,
			((data->codec_configured == A2DP_CODEC_MPEG12)?"SBC_FALLBACK":""));

			// Could not configure MP3 Codec.
			// Safely fallback to SBC for this session.
			data->sbc_fallback = 1;
			data->codec_configured = 0;
			goto error;
		}
	} else
#endif //ENABLE_MP3_ITTIAM_CODEC
	if ( (data->codec_supported_remote & (1 << BT_A2DP_SBC_SINK)) != 0) {
		DBG("sbc codec found : calling bluetooth_a2dp_hw_params");
		DBG(" data->codec_priority = %d",data->codec_priority);
		data->codec_configured = A2DP_CODEC_SBC;
		err = bluetooth_a2dp_hw_params(data);
		if (err < 0) {
			ERR("bluetooth_a2dp_sbc_hw_params failed err: %d", err);
			goto error;
		}		
	} else {
		ERR("bluetooth_configure SBC is not supported");
		goto error;
	}

	set_state(data, A2DP_STATE_CONFIGURED);
	return 0;

error:

	if (data->state == A2DP_STATE_CONFIGURING)
		bluetooth_close(data);
	return err;
}

static void set_state(struct bluetooth_data *data, a2dp_state_t state)
{
	data->state = state;
	pthread_cond_signal(&data->client_wait);
}

static void __set_command(struct bluetooth_data *data, a2dp_command_t command)
{
	VDBG("set_command %d\n", command);
	data->command = command;
	pthread_cond_signal(&data->thread_wait);
	return;
}

static void set_command(struct bluetooth_data *data, a2dp_command_t command)
{
	pthread_mutex_lock(&data->mutex);
	__set_command(data, command);
	pthread_mutex_unlock(&data->mutex);
}

/* timeout is in milliseconds */
static int wait_for_start(struct bluetooth_data *data, int timeout)
{
	a2dp_state_t state = data->state;
	struct timeval tv;
	struct timespec ts;
	int err = 0;

#ifdef ENABLE_TIMING
	uint64_t begin, end;
	begin = get_microseconds();
#endif

	gettimeofday(&tv, (struct timezone *) NULL);
	ts.tv_sec = tv.tv_sec + (timeout / 1000);
	ts.tv_nsec = (tv.tv_usec + (timeout % 1000) * 1000L ) * 1000L;

	pthread_mutex_lock(&data->mutex);
	while (state != A2DP_STATE_STARTED) {
		if (state == A2DP_STATE_NONE)
			__set_command(data, A2DP_CMD_INIT);
		else if (state == A2DP_STATE_INITIALIZED)
			__set_command(data, A2DP_CMD_CONFIGURE);
		else if (state == A2DP_STATE_CONFIGURED) {
			__set_command(data, A2DP_CMD_START);
		}
again:
		err = pthread_cond_timedwait(&data->client_wait, &data->mutex, &ts);
		if (err) {
			/* don't timeout if we're done */
			if (data->state == A2DP_STATE_STARTED) {
				err = 0;
				break;
			}
			if (err == ETIMEDOUT)
				break;
			goto again;
		}

		if (state == data->state)
			goto again;

		state = data->state;

		if (state == A2DP_STATE_NONE) {
			err = ENODEV;
			break;
		}
	}
	pthread_mutex_unlock(&data->mutex);

#ifdef ENABLE_TIMING
	end = get_microseconds();
	print_time("wait_for_start", begin, end);
#endif

	/* pthread_cond_timedwait returns positive errors */
	return -err;
}

static void a2dp_free(struct bluetooth_data *data)
{
	pthread_cond_destroy(&data->client_wait);
	pthread_cond_destroy(&data->thread_wait);
	pthread_cond_destroy(&data->thread_start);
	pthread_mutex_destroy(&data->mutex);
#ifdef ENABLE_MP3_ITTIAM_CODEC
	if (data->pending_buffer_mp3 != NULL) {
		free(data->pending_buffer_mp3);
	}
	if (data->p_mp3_enc_handle != NULL) {
		ia_mp3_enc_ocp_deinit(data->p_mp3_enc_handle);
	}
	if (data->mpeg12pl_handle != NULL) {
		ia_mpeg12pl_delete(data->mpeg12pl_handle);
	}
#endif //ENABLE_MP3_ITTIAM_CODEC
#ifdef ENABLE_CSR_APTX_CODEC
	if (data->aptxCodec!=NULL) {
		free(data->aptxCodec);
		data->aptxCodec=NULL;
	}
#endif //ENABLE_CSR_APTX_CODEC
	free(data);
	return;
}

static void* a2dp_thread(void *d)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	a2dp_command_t command = A2DP_CMD_NONE;
	int err = 0;

	DBG("a2dp_thread started");
	prctl(PR_SET_NAME, (int)"a2dp_thread", 0, 0, 0);

	pthread_mutex_lock(&data->mutex);

	data->started = 1;
	pthread_cond_signal(&data->thread_start);

	while (1)
	{
		while (1) {
			pthread_cond_wait(&data->thread_wait, &data->mutex);

			/* Initialization needed */
			if (data->state == A2DP_STATE_NONE &&
				data->command != A2DP_CMD_QUIT) {
				err = bluetooth_init(data);
			}

			/* New state command signaled */
			if (command != data->command) {
				command = data->command;
				break;
			}
		}

		switch (command) {
			DBG("a2dp_thread cmd %d state %d",command,data->state);
			case A2DP_CMD_CONFIGURE:
				if (data->state != A2DP_STATE_INITIALIZED)
					break;
				err = bluetooth_configure(data);
				break;

			case A2DP_CMD_START:
				if (data->state != A2DP_STATE_CONFIGURED)
					break;
				err = bluetooth_start(data);
				break;

			case A2DP_CMD_STOP:
				if (data->state != A2DP_STATE_STARTED)
					break;
				err = bluetooth_stop(data);
				break;

			case A2DP_CMD_QUIT:
				bluetooth_close(data);
				sbc_finish(&data->sbc);
				a2dp_free(data);
				data = NULL;
				goto done;

			case A2DP_CMD_INIT:
				/* already called bluetooth_init() */
			default:
				break;
		}
		// reset last command in case of error to allow
		// re-execution of the same command
		if (err < 0) {
			command = A2DP_CMD_NONE;
		}
		if (command)
			DBG("a2dp_thread cmd done state %d", data->state);
	}

done:
	if (data)
		pthread_mutex_unlock(&data->mutex);
	DBG("a2dp_thread finished");
	return NULL;
}

int a2dp_init(int rate, int channels, a2dpData* dataPtr)
{
	struct bluetooth_data* data;
	pthread_attr_t attr;
	int err;

	DBG("a2dp_init rate: %d channels: %d", rate, channels);
	*dataPtr = NULL;
	data = malloc(sizeof(struct bluetooth_data));
	if (!data)
		return -1;

	memset(data, 0, sizeof(struct bluetooth_data));
	data->server.fd = -1;
	data->stream.fd = -1;
	data->state = A2DP_STATE_NONE;
	data->command = A2DP_CMD_NONE;
#ifdef ENABLE_MP3_ITTIAM_CODEC
	data->sbc_fallback = 0;
#endif //ENABLE_MP3_ITTIAM_CODEC
	strncpy(data->address, "00:00:00:00:00:00", 18);
	data->rate = rate;
	data->channels = channels;

	sbc_init(&data->sbc, 0);

	pthread_mutex_init(&data->mutex, NULL);
	pthread_cond_init(&data->thread_start, NULL);
	pthread_cond_init(&data->thread_wait, NULL);
	pthread_cond_init(&data->client_wait, NULL);

	pthread_mutex_lock(&data->mutex);
	data->started = 0;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	err = pthread_create(&data->thread, &attr, a2dp_thread, data);
	if (err) {
		/* If the thread create fails we must not wait */
		pthread_mutex_unlock(&data->mutex);
		err = -err;
		goto error;
	}

	/* Make sure the state machine is ready and waiting */
	while (!data->started) {
		pthread_cond_wait(&data->thread_start, &data->mutex);
	}

	/* Poke the state machine to get it going */
	pthread_cond_signal(&data->thread_wait);

	pthread_mutex_unlock(&data->mutex);
	pthread_attr_destroy(&attr);

	*dataPtr = data;
	return 0;
error:
	bluetooth_close(data);
	sbc_finish(&data->sbc);
	pthread_attr_destroy(&attr);
	a2dp_free(data);

	return err;
}

void a2dp_set_sink(a2dpData d, const char* address)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	if (strncmp(data->address, address, 18)) {
		strncpy(data->address, address, 18);
		set_command(data, A2DP_CMD_INIT);
	}
}

#ifdef ENABLE_CSR_APTX_CODEC
static int avdtp_write_btaptx(struct bluetooth_data *data)
{
	int ret = 0;
	uint64_t now;
	long duration = (10000*4 * data->frame_count) / 441;  // delayed the division by 441
#ifdef ENABLE_TIMING
	uint64_t begin, end, begin2, end2;
	begin = get_microseconds();
#endif
	data->stream.revents = 0;
#ifdef ENABLE_TIMING
	begin2 = get_microseconds();
#endif
	ret = poll(&data->stream, 1, POLL_TIMEOUT);
#ifdef ENABLE_TIMING
	end2 = get_microseconds();
	print_time("poll", begin2, end2);
#endif
	if (ret == 1 && data->stream.revents == POLLOUT) {
		long ahead = 0;
		now = get_microseconds();

		if (data->next_write) {
			ahead = data->next_write - now;
#ifdef ENABLE_TIMING
			DBG("duration: %ld, ahead: %ld", duration, ahead);
#endif
			if (ahead > 0) {
				/* too fast, need to throttle */
				usleep(ahead);
			}
		} else {
			data->next_write = now;
		}
		if (ahead <= -CATCH_UP_TIMEOUT * 1000) {
			/* fallen too far behind, don't try to catch up */
			VDBG("ahead < %d, reseting next_write timestamp", -CATCH_UP_TIMEOUT * 1000);
			data->next_write = 0;
		} else {
			data->next_write += duration;
		}

#ifdef ENABLE_TIMING
		begin2 = get_microseconds();
#endif
		ret = send(data->stream.fd, data->buffer, data->count, MSG_NOSIGNAL);

#ifdef ENABLE_TIMING
		end2 = get_microseconds();
		print_time("send", begin2, end2);
#endif
		if (ret < 0) {
			/* can happen during normal remote disconnect */
			VDBG("send() failed: %d (errno %s)", ret, strerror(errno));
		}
		if (ret == -EPIPE) {
			bluetooth_close(data);
		}
	} else {
		 /* can happen during normal remote disconnect */
		VDBG("poll() failed: %d (revents = %d, errno %s)",
				ret, data->stream.revents, strerror(errno));
		data->next_write = 0;
	}

	/* Reset buffer of data to send */
	data->count = 0; // btaptx has no header....
	data->frame_count = 0;
	data->samples = 0;
	data->seq_num++;
#ifdef ENABLE_TIMING
	end = get_microseconds();
	print_time("avdtp_write", begin, end);
#endif
	return 0; /* always return success */
}

#endif //ENABLE_CSR_APTX_CODEC

int a2dp_write(a2dpData d, const void* buffer, int count)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	uint8_t* src = (uint8_t *)buffer;
	int codesize;
	int err, ret = 0;
	int err_code = 0;
	long frames_left = count;
	int encoded;
	unsigned int written;
	const char *buff;
	int did_configure = 0;

#ifdef ENABLE_MP3_ITTIAM_CODEC
	ia_mp3_enc_input_args_t input_args;
	ia_mp3_enc_output_args_t output_args;
	WORD8 *inp_buffer, *out_buffer;
	signed int input_buff_size, output_buff_size ;
	int rtp_pl_status = 0;
	WORD8 *inputBuffer;
	WORD32 input_size;
	WORD32 bytesConsumed = 0, packetLength;
#endif //ENABLE_MP3_ITTIAM_CODEC

#ifdef ENABLE_CSR_APTX_CODEC
	uint16_t* srcptr = (uint16_t *)buffer;
	uint32_t pcmL[4];
	uint32_t pcmR[4];
	uint16_t encodedSample[2];
	uint8_t t;
#endif //ENABLE_CSR_APTX_CODEC


#ifdef ENABLE_TIMING
	uint64_t begin, end;
	begin = get_microseconds();
#endif

	err = wait_for_start(data, WRITE_TIMEOUT);
	if (err < 0)
		return err;

	codesize = data->codesize;

	VDBG("data->codec_configured = %d ",data->codec_configured);

#ifdef ENABLE_CSR_APTX_CODEC
	if (data->codec_configured == A2DP_CODEC_BTAPTX) {
		codesize = 16;
		written = 4;
		encoded = 8;
		DBG("aptx encoding");
		while (frames_left >= codesize) {
			// get the next 4 PCM samples per channel from the buffer
			for (t = 0; t < 4; t++) {
				pcmL[t] = (uint32_t*)(srcptr[2*t]);
				pcmR[t] = (uint32_t*)(srcptr[2*t+1]);
			}

			aptxbtenc_encodestereo(data->aptxCodec, pcmL, pcmR,  &encodedSample);

			data->buffer[data->count]   = (uint8_t)((encodedSample[0] >>8) & 0xff);
			data->buffer[data->count+1] = (uint8_t)((encodedSample[0] >>0) & 0xff);
			data->buffer[data->count+2] = (uint8_t)((encodedSample[1] >>8) & 0xff);
			data->buffer[data->count+3] = (uint8_t)((encodedSample[1] >>0) & 0xff);

			srcptr += encoded;
			data->count += written;
			data->frame_count++;
			data->samples += encoded;
			data->nsamples += encoded;

			/* No space left for another frame then send
			using max of mtu approx 892 of data->link_mtu (approx 895)
			*/

			if ((data->count + 4 >= data->link_mtu) || (data->count >= 352) ||
				(data->count + 4 >= BUFFER_SIZE)) {

				DBG("from btaptx sending packet %d, data->samples=%d, count %d, link_mtu %u",
					data->seq_num, data->samples, data->count,
					data->link_mtu);

				err = avdtp_write_btaptx(data);
				if (err < 0)
					return err;
			}

			ret += codesize;
			frames_left -= codesize;

		}
	//end btaptx
	} else
#endif

#ifdef ENABLE_MP3_ITTIAM_CODEC
	if (data->codec_configured == A2DP_CODEC_MPEG12) {
		VDBG("MP3 encoding");
		input_args.size = sizeof(ia_mp3_enc_input_args_t);
		output_args.size = sizeof(ia_mp3_enc_output_args_t);
		out_buffer = data->buffer;
		inputBuffer = data->pending_buffer_mp3;

		//Copy to internal buffer from src buffer
		memcpy(data->pending_buffer_mp3+data->filledLen_pending_buffer_mp3,
									src, frames_left);
		data->filledLen_pending_buffer_mp3 += frames_left ;
		VDBG("Internal Input Buffer Filled Len in the beginning : %d",
						data->filledLen_pending_buffer_mp3);

	while ( data->filledLen_pending_buffer_mp3 >= codesize) {

		int numChannels = 2;
		input_size = codesize;
		VDBG("size of data->buffer :%d", sizeof(data->buffer));

		input_args.numInSamples =( codesize / numChannels) / sizeof(short);
		VDBG("MP3 encoding : numInSamples = %d , input_size = %d",
					input_args.numInSamples, input_size);

		err_code =  ia_mp3_enc_ocp_process (data->p_mp3_enc_handle,
							&input_args,
							inputBuffer ,
							input_size , &output_args,
			data->buffer + data->count -1 + data->mpeg12pl_audio_header_size,
			sizeof(data->buffer) - (data->count -1)+ data->mpeg12pl_audio_header_size);

		if(err_code != 0) {
			ERR("MP3 Codec process failed with error:%#x", err_code);
			return err_code;
		} else
			VDBG("MP3 Encoder process success ");

		encoded  = output_args.numInSamples *
				(data->mp3_enc_params.channelMode + 1) * sizeof(short);
		written = output_args.bytesGenerated;

		VDBG("MP3 encoding : encoded out bytes = %d bytes Generated = %d",
									encoded, written);

		data->filledLen_pending_buffer_mp3 -= encoded;
		if(encoded > 0)
			memmove(data->pending_buffer_mp3,data->pending_buffer_mp3 + encoded,
							 data->filledLen_pending_buffer_mp3 );

		VDBG("Internal Input Buffer Filled Len: %d", data->filledLen_pending_buffer_mp3);
		data->samples += encoded;
		data->nsamples += 1;
		data->frame_count++;

		do {
		//Now add RTP Header
		rtp_pl_status = ia_mpeg12pl_add_rtp_header( data->mpeg12pl_handle,
				data->buffer+data->count-1 + data->mpeg12pl_audio_header_size,
				written, &(data->mpeg12pl_packet));

		if (rtp_pl_status == MPEG12PL_FRAG_COMPLETE
			|| rtp_pl_status  == MPEG12PL_FRAG_PROG) {
			VDBG("RTP payload header addition success ");
		} else if (rtp_pl_status  != 0) {
			ERR("RTP payload header addition failed with error:%#x", rtp_pl_status );
			return rtp_pl_status;
		}

		VDBG("Copy from payload ptr 0x%x to data->buffer, length: %d\n",
			data->mpeg12pl_packet.payload_ptr, data->mpeg12pl_packet.payload_len);
		memcpy(data->buffer+data->count-1, data->mpeg12pl_packet.payload_ptr,
							data->mpeg12pl_packet.payload_len);

		if ( rtp_pl_status == MPEG12PL_FRAG_COMPLETE) {
			VDBG("Fragmentation Complete payload length :%d",
						data->mpeg12pl_packet.payload_len );
			data->count += data->mpeg12pl_packet.payload_len -1 ;
			if ( (data->count + written  >= data->link_mtu) ||
					(data->count + written >= BUFFER_SIZE)) {
				VDBG("from MP3 sending packet %d, data->samples=%d,\
						count %d, link_mtu %u",	data->seq_num,
						data->samples, data->count, data->link_mtu);

				err = avdtp_write(data);
				if (err < 0)
					return err;
			}
		} else {
			//Data is more than link_mtu, so send it right away
			VDBG("Fragmentation in progress current payload length :%d",
							data->mpeg12pl_packet.payload_len );
			data->count += data->mpeg12pl_packet.payload_len-1 ;
			VDBG("MP3 sending packet %d, data->samples=%d, count %d, link_mtu %u",
					data->seq_num, data->samples, data->count,data->link_mtu);
			err = avdtp_write(data);
			if (err < 0)
				return err;
		}
		VDBG("data->mpeg12pl_audio_header_size : %d", data->mpeg12pl_audio_header_size );

		} while (rtp_pl_status != MPEG12PL_FRAG_COMPLETE);
		ret= frames_left;
		VDBG("MP3 encoding : TOTAL encoded out bytes = %d",ret);

	}
	} else
#endif //ENABLE_MP3_ITTIAM_CODEC
	if (data->codec_configured == A2DP_CODEC_SBC) {
		VDBG("SBC encoding");

		while (frames_left >= codesize) {
			/* Enough data to encode (sbc wants 512 byte blocks) */
			encoded = sbc_encode(&(data->sbc), src, codesize,
					data->buffer + data->count,
					sizeof(data->buffer) - data->count,
					&written);
			if (encoded <= 0) {
				ERR("Encoding error %d", encoded);
				goto done;
			}
			VDBG("sbc_encode returned %d, codesize: %d, written: %d\n",
							encoded, codesize, written);

			src += encoded;
			data->count += written;
			data->frame_count++;
			data->samples += encoded;
			data->nsamples += encoded;
			/* No space left for another frame then send */
			if ((data->count + written >= data->link_mtu) ||
				(data->count + written >= BUFFER_SIZE)) {
				VDBG("sending packet %d, count %d, link_mtu %u",
						data->seq_num, data->count,
								data->link_mtu);
				err = avdtp_write(data);
				if (err < 0)
					return err;
			}

			ret += encoded;
			frames_left -= encoded;
		}
	} else {
		ERR("No suported codec encoder\n");
	}

	if (frames_left > 0)
		VDBG("%ld bytes left at end of a2dp_write\n", frames_left);

done:
#ifdef ENABLE_TIMING
	end = get_microseconds();
	print_time("a2dp_write total", begin, end);
#endif
	VDBG("Returning from a2dp_writee : %d \n", ret);
	return ret;
}

int a2dp_stop(a2dpData d)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	DBG("a2dp_stop\n");
	if (!data)
		return 0;

	set_command(data, A2DP_CMD_STOP);
	return 0;
}

void a2dp_cleanup(a2dpData d)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	DBG("a2dp_cleanup\n");
	set_command(data, A2DP_CMD_QUIT);
}

#ifdef ENABLE_CSR_APTX_CODEC

/* bluetooth_a2dp_btaptx_init(struct bluetooth_data *data) function for btaptx to initialized
 * NOTE:  In this function we are not checking for vender and codec ids here as while
 * initialization we set  frequency and mode only for btaptx 17.01.2011. */


static int bluetooth_a2dp_btaptx_init(struct bluetooth_data *data)
{
	btaptx_capabilities_t *cap = &data->btaptx_capabilities;
	int dir;

	DBG("We are in bluetooth_a2dp_btaptx_init()\n");

	if ((cap->frequency & BT_BTAPTX_SAMPLING_FREQ_44100) == 0)
	{
		ERR("sampling frequency %d not supported", cap->frequency);
		return -1;
	} else {
		cap->frequency = BT_BTAPTX_SAMPLING_FREQ_44100;
	}

	if ((cap->channel_mode & BT_A2DP_CHANNEL_MODE_STEREO) == 0)
	{
		ERR("No supported channel modes");
		return -1;
	} else {
		cap->channel_mode = BT_A2DP_CHANNEL_MODE_STEREO;
	}

	return 0;
}
/*
        bluetooth_a2dp_btaptx_setup(struct bluetooth_data *data)

*/

static int bluetooth_a2dp_btaptx_setup(struct bluetooth_data *data)
{
	btaptx_capabilities_t active_capabilities = data->btaptx_capabilities;

	DBG("We are in bluetooth_a2dp_btaptx_setup()\n");

	if ((active_capabilities.frequency & BT_BTAPTX_SAMPLING_FREQ_44100) == 0)
	{
		ERR("sampling frequency %d not supported", active_capabilities.frequency);
		return -1;
	} else {
		data->btaptx.frequency = BT_BTAPTX_SAMPLING_FREQ_44100;
	}

	if ((active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_STEREO) == 0)
	{
		ERR("No supported channel modes");
		return -1;
	} else {
		data->btaptx.mode = BT_A2DP_CHANNEL_MODE_STEREO;
	}

	data->aptxCodec = malloc((size_t)SizeofAptxbtenc());
	if (data->aptxCodec == NULL)
	{
		return -1;
	}

	aptxbtenc_init(data->aptxCodec, 0);

	DBG("End of bluetooth_a2dp_btaptx_setup()\n");
	return 0;
}

/*

bluetooth_a2dp_btaptx_hw_params(struct bluetooth_data *data)


*/
static int bluetooth_a2dp_btaptx_hw_params(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_open_req *open_req = (void *) buf;
	struct bt_open_rsp *open_rsp = (void *) buf;
	struct bt_set_configuration_req *setconf_req = (void*) buf;
	struct bt_set_configuration_rsp *setconf_rsp = (void*) buf;
	int err;

	DBG("We are in bluetooth_a2dp_btaptx_hw_params()\n");

	memset(open_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	open_req->h.type = BT_REQUEST;
	open_req->h.name = BT_OPEN;
	open_req->h.length = sizeof(*open_req);
	strncpy(open_req->destination, data->address, 18);

	open_req->seid = data->btaptx_capabilities.capability.seid;
	open_req->lock = BT_WRITE_LOCK;

	DBG("open_req->h.type = BT_REQUEST=%s\n", bt_audio_strname(open_req->h.type));
	DBG("open_req->h.name = BT_OPEN=%s\n", bt_audio_strname(open_req->h.name));
	DBG("open_req->h.length = sizeof(*open_req)=%u\n",open_req->h.length);

	DBG("strncpy(open_req->destination, data->address,18=%H\n",open_req->destination);
	DBG("open_req->seid = data->btaptx.capability.seid=%u\n", open_req->seid);
	DBG("open_req->lock = BT_WRITE_LOCK=%u\n", open_req->lock);

	err = audioservice_send(data, &open_req->h);
	if (err < 0)
		return err;

	open_rsp->h.length = sizeof(*open_rsp);
	err = audioservice_expect(data, &open_rsp->h, BT_OPEN);
	if (err < 0)
		return err;

	err = bluetooth_a2dp_btaptx_init(data);
	if (err < 0)
		return err;
	memset(setconf_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	setconf_req->h.type = BT_REQUEST;
	setconf_req->h.name = BT_SET_CONFIGURATION;
	setconf_req->h.length = sizeof(*setconf_req);

	memcpy(&setconf_req->codec, &data->btaptx_capabilities,
		sizeof(btaptx_capabilities_t));

	setconf_req->codec.transport = BT_CAPABILITIES_TRANSPORT_A2DP;
	setconf_req->codec.length = sizeof(data->btaptx_capabilities);
	setconf_req->h.length += setconf_req->codec.length - sizeof(setconf_req->codec);


	DBG("setconf_req->h.type = BT_REQUEST=%s\n", bt_audio_strname(setconf_req->h.type));
	DBG("setconf_req->h.name = BT_SET_CONFIGURATION=%s\n", bt_audio_strname(setconf_req->h.name));
	DBG("setconf_req->h.length = sizeof(*open_req)=%u\n",setconf_req->h.length);
	DBG("setconf_req->codec.transport=%u\n",setconf_req->codec.transport);
	DBG("setconf_req->codec.length=%u\n", setconf_req->codec.length);
	DBG("setconf_req->h.length += setconf_req->codec.length  - sizeof(setconf_req->codec) (%u %u %u) \n",
		setconf_req->h.length, setconf_req->codec.length, sizeof(setconf_req->codec));

	DBG("bluetooth_a2dp_btaptx_hw_params sending configuration and DBG starts\n");

	DBG("bluetooth_a2dp_btaptx_hw_params sending configuration:\n");

#ifdef DEBUG
	switch (data->btaptx_capabilities.channel_mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
			DBG("\tchannel_mode: MONO\n");
			break;
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			DBG("\tchannel_mode: DUAL CHANNEL\n");
			break;
		case BT_A2DP_CHANNEL_MODE_STEREO:
			DBG("\tchannel_mode: STEREO\n");
			break;
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			DBG("\tchannel_mode: JOINT STEREO\n");
			break;
		default:
			DBG("\tchannel_mode: UNKNOWN (%d)\n",
				data->btaptx_capabilities.channel_mode);
	}
	switch (data->btaptx_capabilities.frequency) {
		case BT_BTAPTX_SAMPLING_FREQ_16000:
			DBG("\tfrequency: 16000\n");
			break;
		case BT_BTAPTX_SAMPLING_FREQ_32000:
			DBG("\tfrequency: 32000\n");
			break;
		case BT_BTAPTX_SAMPLING_FREQ_44100:
			DBG("\tfrequency: 44100\n");
			break;
		case BT_BTAPTX_SAMPLING_FREQ_48000:
			DBG("\tfrequency: 48000\n");
			break;
		default:
			DBG("\tfrequency: UNKNOWN (%d)\n",
				data->btaptx_capabilities.frequency);
	}

	DBG("aptx codec ID0 = %u\n ",  data->btaptx_capabilities.codec_id0);
	DBG("aptx codec ID1 = %u\n ",  data->btaptx_capabilities.codec_id1);
	DBG("aptx Vendor ID0 = %u\n ", data->btaptx_capabilities.vender_id0);
	DBG("aptx Vendor ID1 = %u\n ", data->btaptx_capabilities.vender_id1);
	DBG("aptx Vendor ID2 = %u\n ", data->btaptx_capabilities.vender_id2);
	DBG("aptx Vendor ID3 = %u\n ", data->btaptx_capabilities.vender_id3);
#endif //DEBUG
	err = audioservice_send(data, &setconf_req->h);

	DBG("bluetooth_a2dp_btaptx_hw_params called audioservice_send() from \n");

	if (err < 0)
		return err;

	err = audioservice_expect(data, &setconf_rsp->h, BT_SET_CONFIGURATION);
	DBG("bluetooth_a2dp_hw_btaptx_params called audioservice_expect()\n");
	if (err < 0)
		return err;

	data->link_mtu = setconf_rsp->link_mtu;
	DBG("MTU: %d", data->link_mtu);

	/* Setup BTAPTX encoder now we agree on parameters */

	DBG("bluetooth_a2dp_btaptx_hw_params called bluetooth_a2dp_btaptx_setup(data)\n");

	err = bluetooth_a2dp_btaptx_setup(data);


	DBG("end of bluetooth_a2dp_btaptx_hw_btaptx_params()\n");

	return err;
}
#endif //ENABLE_CSR_APTX_CODEC



