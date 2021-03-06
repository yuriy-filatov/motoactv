/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BLUEDROID_BLUETOOTH_H__
#define __BLUEDROID_BLUETOOTH_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <bluetooth/bluetooth.h>

/* Enable the bluetooth interface.
 *
 * Responsible for power on, bringing up HCI interface, and starting daemons.
 * Will block until the HCI interface and bluetooth daemons are ready to
 * use.
 *
 * Returns 0 on success, -ve on error */
int bt_enable();

/* Disable the bluetooth interface.
 *
 * Responsbile for stopping daemons, pulling down the HCI interface, and
 * powering down the chip. Will block until power down is complete, and it
 * is safe to immediately call enable().
 *
 * Returns 0 on success, -ve on error */
int bt_disable();

/* Returns 1 if enabled, 0 if disabled, and -ve on error */
int bt_is_enabled();

int ba2str(const bdaddr_t *ba, char *str);
int str2ba(const char *str, bdaddr_t *ba);


int enableANT();
int enableLE();
int load_firmware(int fd, const char *firmware);
int read_command_complete_leant(int fd, unsigned short opcode, unsigned char len);
int read_hci_event_leant(int fd, unsigned char* buf, int size);
static int get_hci_sock();


int ble_disable();
int ant_disable();

/* Disable the bluetooth interface, irrespective of how many apps are referring to it
 * Do not call this function, unless required.
 * Its now called as part of Instant On Mode Sequence
 *
 * Responsbile for stopping daemons, pulling down the HCI interface, and
 * powering down the chip. Will block until power down is complete, and it
 * is safe to immediately call enable().
 *
 * Returns 0 on success, -ve on error
 *
 */

int bt_force_disable();

/**
Initialise the mutex variable when BT service is started
*/
void init_bt_mutex();

#ifdef __cplusplus
}
#endif
#endif //__BLUEDROID_BLUETOOTH_H__
