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

#define LOG_TAG "bluedroid"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <bluedroid/bluetooth.h>
#include <pthread.h>

#ifndef HCI_DEV_ID
#define HCI_DEV_ID 0
#endif

#define HCID_START_DELAY_SEC  1
#define HCID_STOP_DELAY_USEC 500000

#define MIN(x,y) (((x)<(y))?(x):(y))

#include <sys/uio.h>


#define DBG(fmt, arg...) LOGD("%s(): " fmt, __FUNCTION__, ##arg)
#define ERR(fmt, arg...) LOGE("%s(): " fmt, __FUNCTION__, ##arg)

//mutex to lock bt enable , disable and force disable calls
pthread_mutex_t bt_mutex;

#ifdef USE_UGLY_POWER_INTERFACE
#define BLUETOOTH_POWER_PATH "/proc/bt_power"

static int set_bluetooth_power(int on) {
    int ret = -1;
    int sz;
    const char buffer = (on ? '1' : '0');
    int fd = open(BLUETOOTH_POWER_PATH, O_WRONLY | O_APPEND);

    if (fd == -1) {
        LOGE("Can't open %s for write: %s (%d)", BLUETOOTH_POWER_PATH,
             strerror(errno), errno);
        goto out;
    }
    sz = write(fd, &buffer, 1);
    if (sz != 1) {
        LOGE("Can't write to %s: %s (%d)", BLUETOOTH_POWER_PATH,
             strerror(errno), errno);
    goto out;
    }
    ret = 0;

out:
    if (fd >= 0) close(fd);
    return ret;
}

static int check_bluetooth_power()
{
    char b[64];

    int fd = open(BLUETOOTH_POWER_PATH, O_RDONLY);

    if(fd == -1)
    {
        printf("Can't open %s for reading: %s\n", BLUETOOTH_POWER_PATH);
        perror(BLUETOOTH_POWER_PATH);
        return(-1);
    }

    if(0 == read(fd, b, 63))
    {
        printf("Can't read from %s\n", BLUETOOTH_POWER_PATH);
        perror(BLUETOOTH_POWER_PATH);
        close(fd);
        return(-1);
    }

    close(fd);

    int r=0; if('1' == *b){r = 1;}
    return(r);
}

#else //USE_UGLY_POWER_INTERFACE

static int rfkill_id = -1;
static char *rfkill_state_path = NULL;

static int init_rfkill() {
    char path[64];
    char buf[16];
    int fd;
    int sz;
    int id;
    for (id = 0; ; id++) {
        snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            LOGW("open(%s) failed: %s (%d)\n", path, strerror(errno), errno);
            return -1;
        }
        sz = read(fd, &buf, sizeof(buf));
        close(fd);
        if (sz >= 9 && memcmp(buf, "bluetooth", 9) == 0) {
            rfkill_id = id;
            break;
        }
    }

    asprintf(&rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", rfkill_id);
    return 0;
}

static int check_bluetooth_power() {
    int sz;
    int fd = -1;
    int ret = -1;
    char buffer;

    if (rfkill_id == -1) {
        if (init_rfkill()) goto out;
    }

    fd = open(rfkill_state_path, O_RDONLY);
    if (fd < 0) {
        LOGE("open(%s) failed: %s (%d)", rfkill_state_path, strerror(errno),
             errno);
        goto out;
    }
    sz = read(fd, &buffer, 1);
    if (sz != 1) {
        LOGE("read(%s) failed: %s (%d)", rfkill_state_path, strerror(errno),
             errno);
        goto out;
    }

    switch (buffer) {
    case '1':
        ret = 1;
        break;
    case '0':
        ret = 0;
        break;
    }

out:
    if (fd >= 0) close(fd);
    return ret;
}

static int set_bluetooth_power(int on) {
    int sz;
    int fd = -1;
    int ret = -1;
    const char buffer = (on ? '1' : '0');

    if (rfkill_id == -1) {
        if (init_rfkill()) goto out;
    }

    fd = open(rfkill_state_path, O_WRONLY);
    if (fd < 0) {
        LOGE("open(%s) for write failed: %s (%d)", rfkill_state_path,
             strerror(errno), errno);
        goto out;
    }
    sz = write(fd, &buffer, 1);
    if (sz < 0) {
        LOGE("write(%s) failed: %s (%d)", rfkill_state_path, strerror(errno),
             errno);
        goto out;
    }
    ret = 0;

out:
    if (fd >= 0) close(fd);
    return ret;
}
#endif //USE_UGLY_POWER_INTERFACE

static inline int create_hci_sock() {
    int sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (sk < 0) {
        LOGE("Failed to create bluetooth hci socket: %s (%d)",
             strerror(errno), errno);
    }
    return sk;
}

struct RefBase_s{
    unsigned int magic;
    unsigned int base;
};

static const char const * REF_FILE = "/tmp/bluedroid_ref";
static const unsigned int MAGIC = 0x55665566;
static int bt_ref_fd = -1;
static int bt_ref_lock()
{
    int ret = -1;
    /*l_type l_whence l_start l_len l_pid*/
    struct flock fl = {F_WRLCK,    SEEK_SET,  0,        0,      0};

    LOGI("Enter bt_ref_lock");
    if ((bt_ref_fd = open(REF_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
    {
        LOGW("open or create bluedroid_ref error");
        ret = -2;
        goto out;
    }

    LOGI("Trying to get lock ....");

    if (fcntl(bt_ref_fd, F_SETLKW, &fl) == -1)
    {
        LOGW("F_SETLKW failed");
        ret = -1;
        goto out;
    }

    LOGI("got lock");
    ret = 0;
out:
    return ret;
}

static int bt_ref_unlock()
{
    int ret = -1;
    LOGI("Enter bt_ref_unlock");
    if(bt_ref_fd != -1) close(bt_ref_fd);
    ret = 0;
    return ret;
}

static int bt_ref_modify(int change)
{
    struct RefBase_s ref = {magic : MAGIC, base : 0};
    int ret = -1;
    int magic = MAGIC;

    LOGI("Enter bt_ref_modify");
	
	/*
	  Change value of 0,comes only during force reset
	  as part of recovery from Instant ON Mode
	  Do not pass 0 , in any other case as it would reset the 
	  reference variable
	*/
	if(change == 0) {
		LOGI("Request to reset the reference to zero");
		ref.magic = MAGIC;
        ref.base = 0;

        lseek(bt_ref_fd, 0, SEEK_SET);

		if (write(bt_ref_fd, (const void*)&ref, sizeof(ref)) == -1)
		{
			LOGE("write reference count failed");
			ret = -1;
			goto out;
		}
		LOGI("Write the reference to zero as part of Reset");		
		ret = ref.base;
		LOGI("Request to reset the reference to zero ,ref = %d", ret);
		goto out;
		
	}

    if (change != 1 && change != -1) {
        LOGE("Invalid change, use only 1 or -1");
        ret = -1;
        goto out;
    }

    if (read(bt_ref_fd, (void *)&ref, sizeof(ref)) == -1)
    {
        LOGW("read reference count error");
        ret = -1;
        goto out;
    }

    if(ref.magic != MAGIC)
    {
        LOGV("reference file magic number is wrong, do reset");
        ref.magic = MAGIC;
        ref.base = 0;
    }

    if(ref.base > 3) LOGE("reference count > 3 base = %d", ref.base);

    ref.base += change;

    lseek(bt_ref_fd, 0, SEEK_SET);

    if (write(bt_ref_fd, (const void*)&ref, sizeof(ref)) == -1)
    {
        LOGE("write reference count failed");
        ret = -1;
        goto out;
    }

    ret = ref.base;
    LOGI("Exit bt_ref_modify ref = %d", ret);
out:
    return ret;
}

int bt_enable() {
    LOGV(__FUNCTION__);

    int ret = -1;
    int refcount = 0;
    int hci_sock = -1;
    int attempt;
	
	LOGI("Entering bt_enable");
	LOGI("bt_enable trying to acquire mutex");
	// Try to acquire a Mutex, if it is already locked , then the requesting thread just sleeps
	pthread_mutex_lock(&bt_mutex);

    bt_ref_lock();
    //1. check whether bt has ben enabled.
    if (bt_is_enabled() == 1) {
        LOGI("bt has been enabled already. inc reference count only");
        //2. Increase reference count
        refcount = bt_ref_modify(1);
        LOGI("after inc : reference count = %d", refcount);
        bt_ref_unlock();
		
		//unlock the mutex held
		pthread_mutex_unlock( &bt_mutex );
		LOGI("bt_enable mutex unlock");
        return 0;
    } 

     
    if (set_bluetooth_power(1) < 0) goto out;
 
    LOGI("Starting hciattach daemon");		
    if (property_set("ctl.start", "hciattach") < 0) {
       LOGE("Failed to start hciattach");
       goto out;
  }


    // Try for 10 seconds, this can only succeed once hciattach has sent the
    // firmware and then turned on hci device via HCIUARTSETPROTO ioctl
    hci_sock = create_hci_sock();
    if (hci_sock < 0) goto out;
    for (attempt = 1000; attempt > 0;  attempt--) {

        if (!ioctl(hci_sock, HCIDEVUP, HCI_DEV_ID)) {
            break;
        }
        usleep(10000);  // 10 ms retry delay
    }
    if (attempt == 0) {
        LOGE("%s: Timeout waiting for HCI device to come up", __FUNCTION__);
        goto out;
    }

    LOGI("Starting bluetoothd daemon");
    if (property_set("ctl.start", "bluetoothd") < 0) {
        LOGE("Failed to start bluetoothd");
        goto out;
    }
    sleep(HCID_START_DELAY_SEC);

    ret = 0;

out:
    if (hci_sock >= 0) close(hci_sock);

    if(0 == ret)
    {
        //3. Increase reference count.
        refcount = bt_ref_modify(1);
        LOGV("after inc : reference count = %d", refcount);
    }
    bt_ref_unlock();
	
	//unlock the mutex held
	pthread_mutex_unlock( &bt_mutex );
	LOGI("bt_enable mutex unlock");
	
    return ret;
}

int enableLE()
{
   LOGV(__FUNCTION__);

  int fd;
  int load_firmware_status;
  char fw[] = "/system/etc/firmware/wl1271_LE.bin";
 /* if (bt_is_enabled() != 1) {
 	if(bt_enable() != 0 ){
 		DBG("BT is not enabled but unable to start BT");
 		return -1;
 	}
 } */
 DBG("BT enabled successfully trying to get hci socket");
 
 fd = get_hci_sock();

 if (fd < 0) {
     DBG("Unable to open hci socket.");
     return -1;
 }
  DBG("inside enableLE() loading firmware ");
  load_firmware_status = load_firmware(fd,fw);
  DBG("inside enableLE() after loading firmware %d ",load_firmware_status);
  return load_firmware_status;

}


int enableANT()
{
  LOGV(__FUNCTION__);

  int fd;
  int load_firmware_status;
  char fw[] = "/system/etc/firmware/wl1271_ANT.bin";
 	if(bt_enable() != 0 ){
 		/* unable to start BT */
 		DBG("BT is not enabled but unable to start BT");
 		return -1;
 	}
 
 DBG("BT enabled successfully trying to get hci socket");
 
 fd = get_hci_sock();

 if (fd < 0) {
     DBG("Unable to open hci socket.");
     return -1;
 }
  DBG("inside enableANT() loading firmware ");
  load_firmware_status = load_firmware(fd,fw);
  DBG("inside enableANT() after loading firmware %d ",load_firmware_status);
  return load_firmware_status;

}

int load_firmware(int fd, const char *firmware) {

	int fw = open(firmware, O_RDONLY);
	if(fw < 0){
		LOGV("Unable to open firmware file");
		DBG("Unable to open firmware file");
		return -1;
		}
	
	LOGV("Uploading firmware...\n");
	DBG("Uploading firmware...%d \n",fd);

	do {
		/* Read each command and wait for a response. */
		unsigned char data[1024];
		unsigned char cmdp[1 + sizeof(hci_command_hdr)];
		hci_command_hdr *cmd = (hci_command_hdr *)(cmdp + 1);
		int nr;
		
		nr = read(fw, cmdp, sizeof(cmdp));
		
		if (!nr)
			break;
		
//		FAILIF(nr != sizeof(cmdp), "Could not read H4 + HCI header!\n");
		if(nr != sizeof(cmdp)){
				LOGV("Could not read H4 + HCI header!\n");
				DBG("Could not read H4 + HCI header!\n");
				return -1;
			}
		//FAILIF(*cmdp != HCI_COMMAND_PKT, "Command is not an H4 command packet!\n");
		if(*cmdp != HCI_COMMAND_PKT){
			LOGV("Command is not an H4 command packet!\n");
			DBG("Command is not an H4 command packet!\n");
			return -1;
			}

		/*FAILIF(read(fw, data, cmd->plen) != cmd->plen,
			   "Could not read %d bytes of data for command with opcode %04x!\n",
			   cmd->plen,
			   cmd->opcode); */

		if(read(fw, data, cmd->plen) != cmd->plen){
			LOGV("Could not read %d bytes of data for command with opcode %04x!\n",
			   cmd->plen,
			   cmd->opcode);
			   
			   			DBG("Could not read %d bytes of data for command with opcode %04x!\n",
			   cmd->plen,
			   cmd->opcode);
			 return -1;

			}

		{
			int nw;
#if 0
			fprintf(stdout, "\topcode 0x%04x (%d bytes of data).\n",
					cmd->opcode,
					cmd->plen);
#endif
			struct iovec iov_cmd[2];
			iov_cmd[0].iov_base = cmdp;
			iov_cmd[0].iov_len	= sizeof(cmdp);
			iov_cmd[1].iov_base = data;
			iov_cmd[1].iov_len	= cmd->plen;
            

			nw = writev(fd, iov_cmd, 2);
			
			/*FAILIF(nw != (int) sizeof(cmd) +	cmd->plen,
				   "Could not send entire command (sent only %d bytes)!\n",
				   nw); */

			if(nw != (int) sizeof(cmd) +	cmd->plen){

				LOGV("Could not send entire command (sent only %d bytes)!\n",
				   nw);
				DBG("Could not send entire command (sent only %d bytes)!\n",
				   nw);

				return -1;
				}
		}
		/*  Wait for response 
		if (read_command_complete_leant( fd, cmd->opcode, cmd->plen) < 0) {
			DBG("read command complete failed ");
			return -1;
		}
 			*/
	} while(1);
//	fprintf(stdout, "Firmware upload successful.\n");
	LOGV("Firmware upload successful.\n");
	DBG("Firmware upload successful.\n");

	close(fw);
	return 0;
}



static int get_hci_sock() {
	int sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	struct sockaddr_hci addr;
	int opt = 1;
 
     if(sock < 0) {
         DBG("Can't create raw socket: %s (%d)", strerror(errno), errno);
         return -1;
     }
 
     if (setsockopt(sock, SOL_HCI, HCI_DATA_DIR, &opt, sizeof(opt)) < 0) {
         DBG("Error setting data direction: %s (%d)", strerror(errno), errno);
         close(sock);
         return -1;
     }
 
     /* Bind socket to the HCI device */
     addr.hci_family = AF_BLUETOOTH;
     addr.hci_dev = 0;  // hci0
     if(bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
         DBG("Can't attach to device hci0: %s (%d)", strerror(errno), errno);
         close(sock);
         return -1;
     }
     return sock;
 }

/* satish */
	

int bt_disable() {
    LOGV(__FUNCTION__);


    int ret = -1;
    int refcount = 0;
    int hci_sock = -1;

    LOGI("bt_disable Entered");

	LOGI("bt_disable trying to acquire mutex");
	// Try to acquire a Mutex, if it is already locked , then the requesting thread just sleeps
	pthread_mutex_lock(&bt_mutex);
	  
    bt_ref_lock();
    //1. Decrease reference count
    refcount = bt_ref_modify(-1);

    //2. Check reference cout
    LOGI("after dec : reference count = %d", refcount);
    if(refcount > 0)
    {
        LOGI("bt_disable: other app using bt so just do noting");
        bt_ref_unlock();
		
		//unlock the mutex held
		pthread_mutex_unlock( &bt_mutex );
		LOGI("bt_disable mutex unlock");
		
        return 0;
    }
    //3. If reference cout == 0, disable bt
    //4. bt still used by other app, return then.

    LOGI("Stopping bluetoothd deamon");
    if (property_set("ctl.stop", "bluetoothd") < 0) {
        LOGE("Error stopping bluetoothd");
        goto out;
    }
    usleep(HCID_STOP_DELAY_USEC);

    hci_sock = create_hci_sock();
    if (hci_sock < 0) goto out;
    ioctl(hci_sock, HCIDEVDOWN, HCI_DEV_ID);
    
    LOGI("Stopping hciattach deamon");
    if (property_set("ctl.stop", "hciattach") < 0) {
       LOGE("Error stopping hciattach");
       goto out;
   }
   if (set_bluetooth_power(0) < 0) {
       goto out;
   }
    ret = 0;

out:
    if (hci_sock >= 0) close(hci_sock);

    bt_ref_unlock();

	//unlock the mutex held
	pthread_mutex_unlock( &bt_mutex );
	LOGI("bt_disable mutex unlock");
	
    return ret;
}

int ble_disable(){
    LOGV(__FUNCTION__);
  DBG("entered ble disabled ");
  
  /* Commenting this code, because while enabling ANT
    LE is disabled first and then ANT is enabled. Hence This code is
	not required.
  */
  
/*     int fd = get_hci_sock();
    hci_command_hdr hdr;
    uint8_t data[] = { 0x00, 0x00 };
    uint8_t type = HCI_COMMAND_PKT;
    int ret, total_len; 


    if (fd < 0) {
       DBG("Unable to open hci socket.");
       return -1;
    }

     hdr.opcode = 0xFD5B; // BLE opcode
     hdr.plen = sizeof(data); // size of data

     struct iovec iov[] = {
              { &type, 1 },
              { &hdr, sizeof(hdr) },
              { &data, sizeof(data) },
          };

     total_len = 1 + sizeof(hdr) + sizeof(data);       

     ret = writev(fd, iov, sizeof(iov)/sizeof(iov[0]));

     if (ret != total_len) {
             LOGE("Failed to write %d bytes (wrote %d): %s (%d)", total_len, ret,
                     strerror(errno), errno);
             DBG("failed to write to hci for ble disabled "); 
             close(fd);
             return -1;
      }
             DBG("success to  write to hci for ble disabled "); 
      close(fd);
 */      return 0;
}

int ant_disable(){
   LOGV(__FUNCTION__);

   DBG("disable ANT enterted ");
   int fd = get_hci_sock();
   hci_command_hdr hdr;
   uint8_t data[] = { 0x00, 0x00, 0x00 };
   uint8_t type = HCI_COMMAND_PKT;
   int ret, total_len;


    if (fd < 0) {
       DBG("Unable to open hci socket.");
       return -1;
    }

     hdr.opcode = 0xFDD0; // AnT opcode
     hdr.plen = sizeof(data); // size of data

     struct iovec iov[] = {
              { &type, 1 },
              { &hdr, sizeof(hdr) },
              { &data, sizeof(data) },
          };

     total_len = 1 + sizeof(hdr) + sizeof(data);    

     ret = writev(fd, iov, sizeof(iov)/sizeof(iov[0]));

     if (ret != total_len) {
             LOGE("Failed to write %d bytes (wrote %d): %s (%d)", total_len, ret,
                     strerror(errno), errno);
             DBG("failed to disable ant ");
             close(fd);
             return -1;
      }
         
             DBG("ant disabled ");
      close(fd);
      
      if (bt_is_enabled() == 1) {
      // bt is enabled disable it 
      DBG("bt is enabled disable it");
      bt_disable();
      }
      return 0;
}

int bt_is_enabled() {
    LOGV(__FUNCTION__);

    int hci_sock = -1;
    int ret = -1;
    struct hci_dev_info dev_info;


    // Check power first
    ret = check_bluetooth_power();
    if (ret == -1 || ret == 0) goto out;

    ret = -1;

    // Power is on, now check if the HCI interface is up
    hci_sock = create_hci_sock();
    if (hci_sock < 0) goto out;

    dev_info.dev_id = HCI_DEV_ID;
    if (ioctl(hci_sock, HCIGETDEVINFO, (void *)&dev_info) < 0) {
        ret = 0;
        goto out;
    }

    ret = hci_test_bit(HCI_UP, &dev_info.flags);

out:
    if (hci_sock >= 0) close(hci_sock);
    return ret;
}

int ba2str(const bdaddr_t *ba, char *str) {
    return sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
                ba->b[5], ba->b[4], ba->b[3], ba->b[2], ba->b[1], ba->b[0]);
}

int str2ba(const char *str, bdaddr_t *ba) {
    int i;
    for (i = 5; i >= 0; i--) {
        ba->b[i] = (uint8_t) strtoul(str, &str, 16);
        str++;
    }
    return 0;
}

/*
Forcefully disable BT as part of instant On mode procedure
Do not call this function in any other modes
*/

int bt_force_disable() {
    LOGV(__FUNCTION__);

    int ret = -1;
    int hci_sock = -1;

	LOGI("bt_force_disable Entered");

	LOGI("bt_force_disable trying to acquire mutex");
	// Try to acquire a Mutex, if it is already locked , then the requesting thread just sleeps
	pthread_mutex_lock(&bt_mutex);
	
    bt_ref_lock();

	/*
	Explicitly reset the reference variable
	This would reset the Magic number and Reference base to 0
	
	DO NOT PASS 0, as a parameter in any other case
	*/
	bt_ref_modify(0);
	
    LOGI("Stopping bluetoothd deamon");
    if (property_set("ctl.stop", "bluetoothd") < 0) {
        LOGE("Error stopping bluetoothd");
        goto out;
    }
    usleep(HCID_STOP_DELAY_USEC);

    hci_sock = create_hci_sock();
    if (hci_sock < 0) goto out;
    ioctl(hci_sock, HCIDEVDOWN, HCI_DEV_ID);

    LOGI("Stopping hciattach deamon");
    if (property_set("ctl.stop", "hciattach") < 0) {
       LOGE("Error stopping hciattach");
       goto out;
   }
   if (set_bluetooth_power(0) < 0) {
       goto out;
   }
    ret = 0;
	LOGI("Return value is %d", ret);

out:
    if (hci_sock >= 0) close(hci_sock);

    bt_ref_unlock();

	//unlock the mutex held
	pthread_mutex_unlock( &bt_mutex );
	LOGI("bt_force_disable mutex unlock");
    return ret;
}

/*
Function used to initialise the Mutex
for synchronization during bt enable and disable
*/
void init_bt_mutex() {

	LOGV(__FUNCTION__);
	
	LOGI("Entering init_bt_mutex");
	pthread_mutex_init( &bt_mutex, NULL );
	LOGI("Exiting init_bt_mutex after getting lock");

}
