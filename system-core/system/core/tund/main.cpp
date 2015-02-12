/*
 * Copyright (C) 2010 Three Laws of Mobility Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <poll.h>
#include <fcntl.h>
#include <dirent.h>

#include <private/android_filesystem_config.h>
#include <sys/prctl.h>
#include <linux/capability.h>

#define LOG_TAG "tund"
#include "cutils/log.h"
#include "cutils/sockets.h"
#include <cutils/properties.h>
#include "ResponseCode.h"

#define TUN_DEVICE_FILE      "/dev/tun"
#define TUN_FAKE_DEVICE_FILE "/dev/null"
#define TUN_INTERFACE_NAME   "tun"

#define TUN_STATUS_PROP_NAME    "vpn.net.tun.status"
#define TUN_STATUS_RESET        "reset"
#define TUN_STATUS_DISCONNECTED "disconnected"
#define TUN_STATUS_CONNECTED    "connected"
// Next two are for reference only. Only set in dhcpclient.c,
// which has its own (identical) #defines.
#define TUN_STATUS_COMPLETE     "complete"
#define TUN_STATUS_EXPIRED      "expired"
// Two minutes should be enough to renew. If we fail we shut down the
// VPN of course.
#define TUN_RENEW_SECONDS_EARLY 120

/*
 * These will be needed post-Froyo, because the headers will migrate to
 * "/system/core/include". For now, directly declare all that is needed.
 *
 * #include <netutils/ifc.h>
 * #include <netutils/dhcp.h>
 */
extern "C" {
  int ifc_init();
  void ifc_close();
  int do_dhcp_as_secondary(char *iname, char *subnets);
  void get_dhcp_tunnel_info(uint32_t *ipaddr, uint32_t *gateway,
                            uint32_t *mask, uint32_t *dns1,
                            uint32_t *dns2, uint32_t *server,
                            uint32_t *lease);
  int do_dhcp_renewal(char *iname);
}

// Fail if maximum would be exceeded.
int safe_append(char *dest, unsigned n, char *src) {
  if ((strlen(dest)+strlen(src)) > n) return -1;

  strcat(dest, src);
  return 0;
}

#define MAX_PACKET_DUMP_SIZE  5000
#define MAX_PACKET_CHUNK_SIZE 50
#define MAX_FRAME_SIZE        5000
#define MAX_FRAME_SIZE_WARN   4000

// Dump a packet to log. For debugging.
void print_packet(unsigned char *packet, int size, int has_tap) {
    unsigned int proto = 0;
    char pktbuf[MAX_PACKET_DUMP_SIZE+10];
    char chunkbuf[MAX_PACKET_CHUNK_SIZE+1];

    LOGV("packet read: %d bytes", size);
    if (size < 4) return;

    // Skip the TAP header---not part of the Ethernet frame.
    if (has_tap) { packet += 4; size -= 4; }

    sprintf(pktbuf, "[r ");
    int byteIx, result = 0;
    for (byteIx=0; byteIx<size; byteIx++) {
        snprintf(chunkbuf, MAX_PACKET_CHUNK_SIZE, "%02x ", packet[byteIx]);
        if ((result = safe_append(pktbuf,
                                  MAX_PACKET_DUMP_SIZE,
                                  chunkbuf)) < 0)
            break;
        if (byteIx == 5) {
            snprintf(chunkbuf, MAX_PACKET_CHUNK_SIZE, " - eth dst\n   ");
            if ((result = safe_append(pktbuf,
                                      MAX_PACKET_DUMP_SIZE,
                                      chunkbuf)) < 0)
                break;
        } else if (byteIx == 11) {
            snprintf(chunkbuf, MAX_PACKET_CHUNK_SIZE, " - eth src\n   ");
            if ((result = safe_append(pktbuf,
                                      MAX_PACKET_DUMP_SIZE,
                                      chunkbuf)) < 0)
                break;
        } else if (byteIx == 17) {
            proto =
              ((unsigned char)packet[12]<<8)+
              (unsigned char)packet[13];
            snprintf(chunkbuf, MAX_PACKET_CHUNK_SIZE,
                     " - eth type (%x), ip size (%d), %s",
                     proto,
                     ((unsigned char)packet[16]<<8)+
                     ((unsigned char)packet[17]),
                     proto == 0x86dd ? "ipv6" :
                     proto == 0x800 ? "ip\n" :
                     proto == 0x806 ? "arp\n" :
                     "??\n");
            if ((result = safe_append(pktbuf,
                                      MAX_PACKET_DUMP_SIZE,
                                      chunkbuf)) < 0)
                break;
            if (proto == 0x86dd)
                break;
        } else
            if (byteIx > 17 &&
                ((byteIx-18) % 16) == 15) {
                snprintf(chunkbuf, MAX_PACKET_CHUNK_SIZE, "\n   ");
                if ((result = safe_append(pktbuf,
                                          MAX_PACKET_DUMP_SIZE,
                                          chunkbuf)) < 0)
                    break;
            }
    }
    snprintf(chunkbuf, MAX_PACKET_CHUNK_SIZE,
             "%s]\n", (result < 0) ? "..." : "");
    // This *is* the error handling code,
    // so no point checking the append result anymore.
    safe_append(pktbuf, MAX_PACKET_DUMP_SIZE, chunkbuf);
    LOGV(pktbuf);
    return;
}

// Enough to fit three ethernet frames.
// (No particular reason for this sizing.)
// NOTE: DO NOT USE THESE OUTSIDE OF THE
// TWO FUNCTIONS BELOW.
unsigned char reassembly_buf[MAX_FRAME_SIZE+4];
int reassembly_bufIx = 0, reassembly_bufFill = 0;

// Empty all data from the reassembly buffer. This
// must happen on every reconnect to the DM, otherwise
// stale data will pollute the stream and throw off
// the framing.
void cleanReassembly(void) {
  reassembly_bufIx = 0;
  reassembly_bufFill = 0;
  memset(reassembly_buf, 0, sizeof(reassembly_buf));
}

// Receive a framed packet, possibly using leftovers in the static buffer.
int receivePhone(int s, unsigned char **packet, int *size, int doRead) {
    // Compact the recv buffer.
    memcpy(reassembly_buf,
           reassembly_buf+reassembly_bufIx,
           reassembly_bufFill-reassembly_bufIx);
    reassembly_bufFill -= reassembly_bufIx;
    reassembly_bufIx = 0;

    if (doRead) {
        int size = recv(s,
                        reassembly_buf+reassembly_bufFill,
                        MAX_FRAME_SIZE+4-reassembly_bufFill, 0);
        if (size < 0) {
            LOGE("sCorp recv: %s (%d)", strerror(errno), reassembly_bufFill);
	    // Just return error and the uplink will be closed.
	    // The interface remains available and configured
	    // (but of course the VPN network will be unreachable).
	    return -1;
        }
        reassembly_bufFill += size;
    }

    if (reassembly_bufFill < 4) return 0;

    int frameSize = ntohl(*(int *)reassembly_buf);
    if (frameSize > MAX_FRAME_SIZE) {
      LOGE("sCorp frame too large (%d)", frameSize);
      return -1;
    }
    if (reassembly_bufFill < (frameSize+4)) return 0;

    *size = frameSize;
    *packet = reassembly_buf+4;

    reassembly_bufIx = frameSize+4;
    return 1;
}

// Close the socket and clean up related state.
void closeUplink(int s) {
  close(s);
  cleanReassembly();
}

#define MAX_RESPONSE 200

// Send a response in FrameworkListener format.
void send_response(int sock, int code, const char *msg, const char *error) {
    char response_buf[MAX_RESPONSE];
    memset(response_buf, '\0', sizeof(response_buf));
    snprintf(response_buf, MAX_RESPONSE-1, "%.3d %s %s", code, msg, error);
    write(sock, response_buf, strlen(response_buf)+1);
}

int poll_on_tap = 0; // do not add tap socket to poll unless explicitly set

// Returns a socket (fd) >= 0. Failure is not an option.
int open_tap_node(void) {
    int sock;
    if ((sock = open(TUN_DEVICE_FILE, O_RDWR)) < 0 ) {
        poll_on_tap = 0;
        LOGE("sTap open(): %s", strerror(errno));
        // In order to support arbitrary kernels, we will
        // just open /dev/null now, and pretend everything
        // was fine.
        if ((sock = open(TUN_FAKE_DEVICE_FILE, O_RDWR)) < 0) {
            LOGE("sTap open(dev_nul): %s", strerror(errno));
            exit(20);
        }
    } else {
        poll_on_tap = 1;
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TAP;
        strncpy(ifr.ifr_name, TUN_INTERFACE_NAME, IFNAMSIZ);
        ioctl(sock, TUNSETNOCSUM, 1);
        if (ioctl(sock, TUNSETIFF, (void *)&ifr) < 0 ) {
            LOGE("sTap ioctl(): %s", strerror(errno));
            exit(21);
        }
    }
    return sock;
}

/* Subprocess maintenance. */
void zap_child_process(pid_t *cpid) {
    if (*cpid >= 0) {
        int status;
        pid_t childpid2 = waitpid(*cpid,
                                  &status, WNOHANG);
        if (childpid2 == *cpid) {
            LOGI("DHCP subprocess exited");
        } else {
            int killresult = kill(*cpid, SIGKILL);
            LOGI("DHCP subprocess was terminated");
        }
        *cpid = -1;
    }
}

#define MAX_PENDING 5
#define MAX_COMMAND 3000

// Set UID to AID_VPN. Retain the CAP_NET_ADMIN capability for /dev/tun.
void android_set_aid_and_cap() {
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    if (setgid(AID_VPN))
        LOGE("Could not setgid to 'vpn'");

    if (setuid(AID_VPN))
        LOGE("Could not setuid to 'vpn'");

    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;
    header.version = _LINUX_CAPABILITY_VERSION;
    header.pid = 0;
    cap.effective = cap.permitted = 1 << CAP_NET_ADMIN | 1 << CAP_NET_RAW;
    cap.inheritable = 0;
    if (capset(&header, &cap))
        LOGE("Failed to set process capabilities");
}

int main() {
    android_set_aid_and_cap();
    SLOGI("3LM tund firing up");

    /*
     * Start the proxying here. Execution will be affected by commands we
     * get, such as connection establishment, or connection reset.
     */
    int sCorp = -1;
    int sCmd_accept = -1;

    // Rain or shine, we'll keep this open, so the "tun" interface is
    // always available (sometimes not configured though).
    int sTap;
    sTap = open_tap_node();

    // Command socket, to be used by system server (or test tools).
    int sCmd;
    if ((sCmd = android_get_control_socket("tund")) < 0) {
        LOGE("sCmd android_get_control_socket(): %s", strerror(errno));
        exit(22);
    }
    if (listen(sCmd, MAX_PENDING) < 0) {
        LOGE("sCmd listen(): %s", strerror(errno));
        exit(23);
    }

    int sTapIx, sCmdIx;
    struct pollfd pollArr[4];
    if (poll_on_tap) {
        sTapIx = 0;
        pollArr[sTapIx].fd = sTap; // from tund/interface
        pollArr[sTapIx].events = POLLIN;
        sCmdIx = 1;
        pollArr[sCmdIx].fd = sCmd; // from system_server
        pollArr[sCmdIx].events = POLLIN;
    } else {
        sTapIx = -1;
        sCmdIx = 0;
        pollArr[sCmdIx].fd = sCmd; // from system_server
        pollArr[sCmdIx].events = POLLIN;
    }

    LOGV("poll loop...");
    unsigned char tapMAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char broadcastMAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    pid_t childpid = -1; /* DHCP subprocess, spawn at most 1 */
    property_set(TUN_STATUS_PROP_NAME, TUN_STATUS_RESET);
    while (1) {
        int numPoll = sCmdIx+1;
	int sCorpIx = -1, sCmd_acceptIx = -1;
	if (sCorp >= 0) {
	    pollArr[numPoll].fd = sCorp;
	    pollArr[numPoll].events = POLLIN;
	    sCorpIx = numPoll;
	    numPoll++;
	}
	if (sCmd_accept >= 0) {
	    pollArr[numPoll].fd = sCmd_accept;
	    pollArr[numPoll].events = POLLIN;
	    sCmd_acceptIx = numPoll;
	    numPoll++;
	}
        if (poll(pollArr, numPoll, -1) < 0) {
            LOGE("poll(): %s", strerror(errno));
            exit(6);
        }

        // Packets from "/dev/tun" are always handled. In the worst case
        // we drop them. In the best case they will reach the corporate 
        // intranet and their target host. If we are using the fake tun device
        // we will skip this.
        if ((sTapIx >= 0) && (pollArr[sTapIx].revents & POLLIN)) {
            unsigned char packet[MAX_FRAME_SIZE+4];
            int size;
            if ((size = read(sTap, packet, MAX_FRAME_SIZE+4)) < 0) {
                LOGE("sTap read(): %s", strerror(errno));
                exit(8);
            }
            if (size > MAX_FRAME_SIZE_WARN)
                LOGI("sTap read(): Frame is large (%d)", size);
            if (size > 18 &&
                (packet[16] != 0x86 || packet[17] != 0xdd)) {
                memcpy(tapMAC, packet+10, 6);
                print_packet(packet, size, 1);
                // We stomp over the first 4 bytes of packet. Those
                // contain a TAP header which we have to discard anyway.
                // This allows us to send the length and data in one
                // shot without copying data around.
                *(int *)packet = htonl(size-4);
		if ((sCorp >= 0) && (send(sCorp, packet, size, 0) < 0)) {
		    LOGW("sCorp send(): %s", strerror(errno));
		    closeUplink(sCorp);
		    sCorp = -1;
                    property_set(TUN_STATUS_PROP_NAME,
                                 TUN_STATUS_DISCONNECTED);
		}
            }
        }

        // We might not be connected to the outside world (in which case
        // we only read & drop incoming packets from "/dev/tun" and listen for
        // further instructions).
        if ((sCorp >= 0) && (pollArr[sCorpIx].revents & POLLIN)) {
            unsigned char *packet;
            int size;
            int doRead = 1;
	    int result = 0;

            while ((result = receivePhone(sCorp,
					  &packet,
					  &size,
					  doRead)) > 0) {
                doRead = 0;
                if (memcmp(tapMAC, packet, 6) == 0 ||
                    memcmp(broadcastMAC, packet, 6) == 0) {
                    print_packet(packet, size, 0);
                    unsigned char header_plus[MAX_FRAME_SIZE+4] =
                      {0, 0, packet[12], packet[13]};
                    memcpy(header_plus+4, packet, size);
                    if (write(sTap, header_plus, size+4) < 0) {
                        LOGE("sTap write(): %s", strerror(errno));
                        exit(7);
                    }
                }
            }
	    if (result < 0) {
	        closeUplink(sCorp);
		sCorp = -1;
                property_set(TUN_STATUS_PROP_NAME,
                             TUN_STATUS_DISCONNECTED);
	    }
        }
        if ((sCorp >= 0) && (pollArr[sCorpIx].revents & (POLLERR|POLLHUP))) {
	    closeUplink(sCorp);
	    sCorp = -1;
	    LOGW("uplink closed");
            property_set(TUN_STATUS_PROP_NAME,
                         TUN_STATUS_DISCONNECTED);
	}

        // Someone connected (with a command).
        if (pollArr[sCmdIx].revents & POLLIN) {
            struct sockaddr peer_addr;
            socklen_t peer_addrlen;
	    if (sCmd_accept >= 0) close(sCmd_accept);
	    sCmd_accept = -1;
            if ((sCmd_accept = accept(sCmd, &peer_addr, &peer_addrlen)) < 0) {
	        sCmd_accept = -1;
		LOGE("Error accepting control connection");
	    } else {
	        LOGI("Received control connection");
	    }
	}

	if ((sCmd_accept >= 0) &&
            (sCmd_acceptIx >= 0) &&
            (pollArr[sCmd_acceptIx].revents & POLLIN)) {
	    char command_buf[MAX_COMMAND];
	    memset(command_buf, '\0', sizeof(command_buf));
	    // We expect a whole command, in a single read..
	    if (read(sCmd_accept, command_buf, MAX_COMMAND-1) > 0) {
		int rc = 0;
		char command[MAX_COMMAND] = "";
		char arg1[MAX_COMMAND] = "";
		char arg2[MAX_COMMAND] = "";
		char arg3[MAX_COMMAND] = "";
		int argcount = sscanf(command_buf, "%s %s %s %s",
				      command, arg1, arg2, arg3);
		LOGI("Received a command ['%s','%s',...]", command, arg1);
		if (!strcmp(command, "reset")) {
		    // Reopen /dev/tun to clear the interface.
		    // Should not be used in general, because this will
		    // kill open sockets in apps...
                    zap_child_process(&childpid);
		    if (sCorp >= 0) closeUplink(sCorp);
		    sCorp = -1;
		    close(sTap);
		    sTap = open_tap_node();
                    property_set(TUN_STATUS_PROP_NAME, TUN_STATUS_RESET);
		} else if (!strcmp(command, "configure")) {
		    if (sCorp < 0) {
			rc = ResponseCode::OperationFailed;
			send_response(sCmd_accept, rc,
				      "no_uplink",
				      "");
		    } else {
                        zap_child_process(&childpid);
			if (childpid < 0) {
			    childpid = fork();
			    if (childpid >= 0) {
			        // We forked.
			        if (childpid == 0) {
				    // Child: run DHCP.
                                    if (sTap >= 0) close(sTap);
                                    if (sCorp >= 0) closeUplink(sCorp);
                                    ifc_init();
                                    int result = do_dhcp_as_secondary("tun",
                                                                      arg1);
                                    ifc_close();
                                    LOGI("do_dhcp_as_secondary: %d", result);
                                    while (result >= 0) {
                                        uint32_t lease;
                                        get_dhcp_tunnel_info(NULL, NULL,
                                                             NULL, NULL,
                                                             NULL, NULL,
                                                             &lease);
                                        if (lease < TUN_RENEW_SECONDS_EARLY) {
                                            lease = 2*TUN_RENEW_SECONDS_EARLY;
                                            LOGW("Tunnel lease too short!");
                                        }
                                        sleep(lease - TUN_RENEW_SECONDS_EARLY);
                                        ifc_init();
                                        result = do_dhcp_renewal("tun");
                                        ifc_close();
                                        LOGI("do_dhcp_renewal: %d", result);
                                    }
				    exit(0);
				} else {
				    // Parent process; just keep going.
				    // int status;
				    // wait(&status);
				    // if (WEXITSTATUS(status) == ...) ...
				}
			    } else {
			        int error = errno;
				if (sCorp >= 0) closeUplink(sCorp);
				sCorp = -1;
                                property_set(TUN_STATUS_PROP_NAME,
                                             TUN_STATUS_DISCONNECTED);
				rc = ResponseCode::OperationFailed;
				send_response(sCmd_accept, rc,
					      "fork_failed",
					      strerror(errno));
			    }
			} else {
                            LOGE("Unexpected: DHCP child still lingering");
                        }
		    }
		} else if (!strcmp(command, "disconnect")) {
		    // Just close the uplink, roughly equivalent to
		    // creating a network blackhole on the tun interface.
                    // Currently not used by the DM. Whenever we need a
                    // disconnect, we also want to clean up the routing info,
                    // so "reset" is the right thing to do, as we are not too
                    // concerned about the VPN being "watertight".
		    if (sCorp >= 0) closeUplink(sCorp);
		    sCorp = -1;
                    property_set(TUN_STATUS_PROP_NAME,
                                 TUN_STATUS_DISCONNECTED);
		} else if (!strcmp(command, "connect")) {
		    // Clean up the old connection.
		    if (sCorp >= 0) closeUplink(sCorp);
		    sCorp = -1;
                    property_set(TUN_STATUS_PROP_NAME,
                                 TUN_STATUS_DISCONNECTED);
		    // Local is the only production option. Remote is
		    // for testing.
		    if (!strcmp(arg1, "local")) {
			if ((sCorp = socket_local_client(arg2,
							 ANDROID_SOCKET_NAMESPACE_ABSTRACT,
							 SOCK_STREAM)) < 0) {
			    int error = errno;
			    sCorp = -1;
			    rc = ResponseCode::OperationFailed;
			    send_response(sCmd_accept, rc,
					  "socket_failed",
					  strerror(error));
			}
			if (sCorp >= 0) {
			    LOGI("uplink connected (%s)", arg2);
                            property_set(TUN_STATUS_PROP_NAME,
                                         TUN_STATUS_CONNECTED);
                        }
		    } else if (!strcmp(arg1, "remote")) {
		        struct sockaddr_in tapSvrAddr;
			memset(&tapSvrAddr, 0, sizeof(tapSvrAddr));
			tapSvrAddr.sin_family = AF_INET;
			if ((sCorp = socket(PF_INET,
					    SOCK_STREAM,
					    IPPROTO_TCP)) < 0) {
			    int error = errno;
			    sCorp = -1;
			    rc = ResponseCode::OperationFailed;
			    send_response(sCmd_accept, rc,
					  "socket_failed",
					  strerror(error));
			}
			if (sCorp >=0 &&
			    !inet_aton(arg2, &(tapSvrAddr.sin_addr))) {
			    closeUplink(sCorp);
			    sCorp = -1;
			    rc = ResponseCode::OperationFailed;
			    send_response(sCmd_accept, rc,
					  "inet_aton_failed", arg2);
			}
			tapSvrAddr.sin_port = htons(atoi(arg3));
			if (sCorp >= 0 &&
			    connect(sCorp,
				    (struct sockaddr *)&tapSvrAddr,
				    sizeof(tapSvrAddr)) < 0) {
			    int error = errno;
			    closeUplink(sCorp);
			    sCorp = -1;
			    rc = ResponseCode::OperationFailed;
			    send_response(sCmd_accept, rc,
					  "connect_failed",
					  strerror(error));
			}
			if (sCorp >= 0) {
                          LOGI("uplink connected (%s:%d)", arg2, atoi(arg3));
                          property_set(TUN_STATUS_PROP_NAME,
                                       TUN_STATUS_CONNECTED);
                        }
		    } else {
		        rc = ResponseCode::CommandSyntaxError;
			send_response(sCmd_accept, rc,
				      "unknown_connect_type", arg1);
		    }
		} else {
		    rc = ResponseCode::CommandSyntaxError;
		    send_response(sCmd_accept, rc,
				  "unknown_command", command);
		}

		if (rc == 0) send_response(sCmd_accept,
					   ResponseCode::CommandOkay,
					   "done", command);
	    } else {
	        close(sCmd_accept);
		sCmd_accept = -1;
	    }
	}
    }

    SLOGI("3LM tund exiting");
    exit(0);
}
