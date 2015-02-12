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

#ifndef MPEG12-PAYLOAD_H
#define MPEG12-PAYLOAD_H

#define MPEG12PL_HANDLE_SIZE 128
#define MPEG12PL_SUCCESS     0

/* Status codes */
#define MPEG12PL_AGGR_PROG        1
#define MPEG12PL_AGGR_FULL        2
#define MPEG12PL_AGGR_COMPLETE    3
#define MPEG12PL_FRAG_PROG        4
#define MPEG12PL_FRAG_COMPLETE    5
#define MPEG12PL_DEAGGR_PROG      6
#define MPEG12PL_DEAGGR_COMPLETE  7
#define MPEG12PL_DEFRAG_PROG      8
#define MPEG12PL_DEFRAG_COMPLETE  9


/* Indicates the input parameter state                                */

/* Parameter declared with IN, Holds Input value                      */
#define  IN

/* Parameter declared with OUT, will be used to hold output value     */
#define  OUT

/* Parameter declared with INOUT, will have input value and will hold */
/* output value */
#define  INOUT

#define MPEG12PL_ERROR_BASE                    0x92000

#define MPEG12PL_INVALID_INPUT                 (MPEG12PL_ERROR_BASE + 0x01)
#define MPEG12PL_PAYLOAD_SIZE_ERROR            (MPEG12PL_ERROR_BASE + 0x02)
#define MPEG12PL_FRAME_TOO_BIG_FOR_AGGREGATION (MPEG12PL_ERROR_BASE + 0x03)
#define MPEG12PL_PACKET_MISS                   (MPEG12PL_ERROR_BASE + 0x04)
#define MPEG12PL_MODE_NOT_SUPPPORTED           (MPEG12PL_ERROR_BASE + 0x05)
#define MPEG12PL_INCOMPLETE_DU                 (MPEG12PL_ERROR_BASE + 0x06)
#define MPEG12PL_INSUFFICIENT_OUTPUT_BUFFER    (MPEG12PL_ERROR_BASE + 0x07)

#define MPEG12PL_UNKNOWN_ERROR                 (MPEG12PL_ERROR_BASE + 0xFF)

/* This enum is used to indicate data payload type in the packet */
typedef enum
{
    MPEG12PL_MPEG12_AUDIO,
    MPEG12PL_MPEG1_VIDEO,
    MPEG12PL_MPEG2_VIDEO,
    MPEG12PL_MPEG2_TS
} MPEG12PL_PAYLOAD_TYPE_T;

/* This enum is used to indicate the usage mode supported in the library */
typedef enum
{
    MPEG12PL_SINGLE_AU,
    MPEG12PL_SPLIT_AU,
    MPEG12PL_SINGLE_AU_AGGR,
    MPEG12PL_SINGLE_AU_FRAG,
    MPEG12PL_SINGLE_AU_AGGR_FRAG
} MPEG12PL_USAGE_MODE_T;

/* This enum is used to select the de-packetization mode of RTP packets */
typedef enum
{
    MPEG12PL_DU_PACKET,
    MPEG12PL_DU_SLICE,
    MPEG12PL_DU_FRAME
} MPEG12PL_DU_MODE_T;

/* Initialization attributes for library */
typedef struct
{
    UWORD32                 max_payload_len; /* Maximum RTP payload length   */
    MPEG12PL_USAGE_MODE_T   usage_mode;      /* Packetization mode           */
    MPEG12PL_PAYLOAD_TYPE_T payload_type;    /* Payload type                 */
    MPEG12PL_DU_MODE_T      du_mode;         /* De-packetization mode        */
} mpeg12pl_attr_t;

/* This is a structure that should be used by the 'application / system' to  */
/* pass the callback details to the MPEG12PL                                 */
typedef struct
{
    void (*mpeg12pl_mem_copy)(void    *cb_params,
                              void    *src,
                              void    *dst,
                              UWORD32 num_bytes);
    void *mpeg12pl_cm_params;

    void *(*mpeg12pl_alloc)(void    *cb_params,
                            UWORD32 size);
    void *mpeg12pl_al_params;

    void (*mpeg12pl_free)(void *cb_params,
                          void *memory_ptr);

    void *mpeg12pl_fr_params;

} mpeg12pl_cb_funcs_t;

/* This is a structure used by the application / system to store packet      */
/* parameters to pass to the library or to get from the library. This        */
/* encapsulates the RTP payload.                                             */
typedef struct
{
    UWORD8  *payload_ptr;             /* Pointer to the start of RTP payload */
    UWORD32 payload_len;              /* RTP Payload length                  */
    UWORD8  marker;                   /* Marker bit                          */
    UWORD16 seq_num;                  /* Sequence number                     */
    UWORD8  is_current_packet_parsed; /* Used to indicate current packet is  */
                                      /* parsed or not.                      */
} mpeg12pl_packet_t;

/* This structure is used to store mpeg1/mpeg2 audio payload parameters  */
typedef struct
{
    UWORD8  *bitstream_ptr; /* Pointer to the start of audio bitstream */
    UWORD32 bitstream_len;  /* Audio bitstream length in bytes         */
} mpeg12pl_audio_params_t;

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

/* Initializes the payload library and fills in the handle. This handle is   */
/* to be used by any of the subsequent calls. Memory allocation to the       */
/* handle is assumed to be done by the application / system.                 */
extern WORD32 mpeg12pl_init(IN void            *mpeg12pl_handle,
                            IN mpeg12pl_attr_t *attr);

/* Terminates the library and releases any resources that the library has    */
/* acquired.                                                                 */
extern WORD32 mpeg12pl_close(IN void *mpeg12pl_handle);

/* This API is used to register call back functions with MPEG12PL */
extern WORD32 mpeg12pl_register_callbacks(
                                      IN void                *mpeg12pl_handle,
                                      IN mpeg12pl_cb_funcs_t *cb_funcs);


/* Calculates the maximum possible memory offset that may be needed by the   */
/* library to add the payload header and returns that. The system should use */
/* this to allocate sufficient memory before the start of the bitstream      */
/* incase of SINGLE_AU and SPLIT_AU usage modes.                             */
extern WORD32 mpeg12pl_get_header_offset(IN  void   *mpeg12pl_handle,
                                         OUT UWORD8 *header_size);

/* Constructs the payload headers for the bitstream using parameters passed. */
/* It is assumed that enough memory is available before the  start of the    */
/* bitstream passed as input in "params" for the payload headers to be added */
/* incase of SINGLE_AU and SPLIT_AU usage modes.                             */
extern WORD32 mpeg12pl_add_header(IN    void              *mpeg12pl_handle,
                                  IN    void              *params,
                                  INOUT mpeg12pl_packet_t *packet);


#endif /* MPEG12-PAYLOAD_H */
