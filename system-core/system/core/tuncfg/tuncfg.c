/*
 * Copyright (C) Three Laws of Mobility Inc.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * These will be needed post-Froyo, because the headers will migrate to
 * "/system/core/include". For now, directly declare all that is needed.
 *
 * #include <netutils/ifc.h>
 * #include <netutils/dhcp.h>
 */
int ifc_init();
void ifc_close();
int do_dhcp_as_secondary(char *iname);

int main() {
  ifc_init();
  int result = do_dhcp_as_secondary("tun");
  ifc_close();
  printf("do_dhcp_as_secondary: %d\n", result);
  return result;
}
