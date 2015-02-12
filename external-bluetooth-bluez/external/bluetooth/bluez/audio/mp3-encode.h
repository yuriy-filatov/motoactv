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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IA_MP3_ENC_OMX_H
#define IA_MP3_ENC_OMX_H

typedef signed char		WORD8;
typedef unsigned char		UWORD8;
typedef unsigned short		UWORD16;
typedef signed int		WORD32;
typedef unsigned int		UWORD32;
typedef void			VOID;

typedef enum {
	MP3_BITRATE_FREE = 0,
	MP3_BITRATE_32,
	MP3_BITRATE_40,
	MP3_BITRATE_48,
	MP3_BITRATE_56,
	MP3_BITRATE_64,
	MP3_BITRATE_80,
	MP3_BITRATE_96,
	MP3_BITRATE_112,
	MP3_BITRATE_128,
	MP3_BITRATE_160,
	MP3_BITRATE_192,
	MP3_BITRATE_224,
	MP3_BITRATE_256,
	MP3_BITRATE_320,
	MP3_BITRATE_INVALID
} Mpeg3BitrateIndex;

typedef struct {
	int	ver;		//1:MPEG-1, 2:MPEG-2, 3:MPEG-2.5
	int	layer;
	int	crc;
	int	br_index;	//bitrate in kbps.
	int	fr_index;
	int	padding;
	int	extension;
	int	mode;
	int	mode_ext;
	int	copyright;
	int	original;
	int	emphasis;
} MpegHeader;

/*
** Params Structure : This structure defines the creation parameters
** for all aac objects
*/
typedef struct ia_mp3_enc_params_t {
	WORD32 size;
	WORD32 sampleRate;
	WORD32 bitRate;
	WORD32 channelMode;
	WORD32 dataEndianness;
	WORD32 encMode;
	WORD32 inputFormat;
	WORD32 inputBitsPerSample;
	WORD32 maxBitRate;
	WORD32 dualMonoMode;
	WORD32 crcFlag;
	WORD32 ancFlag;
	WORD32 lfeFlag;
	WORD32 packet;
} ia_mp3_enc_params_t;

/*
** Input args structure: Defines runtime input arguments for process function.
*/
typedef struct ia_mp3_enc_input_args_t {
	UWORD32 size;		// size field
	WORD32 numInSamples;	// Size of input data in bytes
}ia_mp3_enc_input_args_t;

/*
** Output args structure: Defines runtime output arguments for process function.
*/
typedef struct ia_mp3_enc_output_args_t
{
	WORD32 size;
	WORD32 extendedError;
	WORD32 bytesGenerated;
	WORD32 numZeroesPadded;
	WORD32 numInSamples;
	WORD32 i_exec_done;
	WORD32 i_ittiam_err_code;
}ia_mp3_enc_output_args_t;

/* Fuction declarations for MP3 Encoder */
WORD32 ia_mp3_enc_ocp_init(VOID **handle, ia_mp3_enc_params_t *mp3_enc_params);
WORD32 ia_mp3_enc_ocp_process(VOID *handle,	ia_mp3_enc_input_args_t *input_args,
						WORD8 *input_buffer,
						WORD32 input_buff_size,
						ia_mp3_enc_output_args_t *output_args,
						WORD8 *output_buffer,
						WORD32 output_buff_size);
//WORD32 ia_mp3_enc_ocp_deinit(VOID *handle);

#endif

#ifdef __cplusplus
}
#endif
