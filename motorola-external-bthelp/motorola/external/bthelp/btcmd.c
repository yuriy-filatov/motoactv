/*
 *  Utility function to support Motorola use-cases on Motorola hardware.
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <fcntl.h>

#define AP_BT_DATA_FILE_ROM                    "/system/etc/firmware/ap_bt_data.bin"
#define PCM_FREQ_CDMA_IN_CALL                  2048
#define PCM_FREQ_CDMA_OUT_OF_CALL              512

struct bt_clk_data {
    unsigned char params_512[34];
    unsigned char params_2048[34];
    unsigned short ocf;
    unsigned char ogf;
};

struct bt_clk_data bt_param;

int bt_data_table_init( const char* filename )
{
    int fd = -1;

    if (filename == NULL) {
        printf("Invalid NULL filename for bt data file\n");
        return -1;
    }
    
    if ((fd = open(filename, O_RDONLY)) == -1) {
        printf("Failed to open gains file %s\n", filename);
        return -1;
    }

    /* Read the data to our variable */
    int file_size;
    file_size = read(fd, &bt_param, sizeof(bt_param));

    /* check the file size */
    if(file_size != sizeof(bt_param) - 1) {
        close(fd);
        return -1;
    }

    /* Print out the values from the file to validate */
    printf(" ogf = %x; ocf = %x\n", bt_param.ogf, bt_param.ocf);

    close(fd);

    return 0;
}

/* Usage: btcmd <PCM bit clock in kHz (decimal)> */

int main(int argc, char *argv[])
{
	int dd;
	unsigned short pcm_freq;
	unsigned char *ptr;
	unsigned char ogf;
	unsigned short ocf;
    unsigned char *params;
    int table_init_ok = -1;
    
    table_init_ok = bt_data_table_init(AP_BT_DATA_FILE_ROM );
    if (table_init_ok !=0) {
        printf("ap_bt_data file is not being init ok\n");
        return -1;
    }
    
    if (argc < 2) {
		printf("Usage: btcmd <PCM bit clock in kHz (decimal)>\n");
		return -1;
	}

	/* Set desired PCM clock in params[] in little-endian */
	pcm_freq = (unsigned short) strtol(argv[1], NULL, 10);

    printf("pcm_freq is %d\n", pcm_freq);

    if (pcm_freq == PCM_FREQ_CDMA_OUT_OF_CALL) {
        params = bt_param.params_512;
    }
    else if (pcm_freq == PCM_FREQ_CDMA_IN_CALL) {
        params = bt_param.params_2048;
    }

	dd = hci_open_dev(0);
	if (dd < 0) {
		printf("Failed to open device\n");
		return -1;
	}

	if (hci_send_cmd(dd, bt_param.ogf, bt_param.ocf, 34, params) < 0) {
		printf("Failed to send command\n");
		hci_close_dev(dd);
		return -1;
	}

	hci_close_dev(dd);
	return 0;
}

