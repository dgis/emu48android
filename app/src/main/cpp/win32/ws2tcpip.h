#ifndef TRUNK_WS2TCPIP_H
#define TRUNK_WS2TCPIP_H

#include <sys/endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/tcp.h>
#include <netinet/in6.h>
#include <sys/select.h>

typedef struct sockaddr_in6 SOCKADDR_IN6_LH;
typedef SOCKADDR_IN6_LH *PSOCKADDR_IN6;
typedef struct in6_addr *PIN6_ADDR;


#define IOCPARM_MASK    0x7f            /* parameters must be < 128 bytes */
#define IOC_IN          0x80000000      /* copy in parameters */
#define _IOW(x,y,t)     (IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
#define FIONBIO     _IOW('f', 126, u_long) /* set/clear non-blocking i/o */

typedef struct timeval TIMEVAL;
typedef fd_set FD_SET;

//#define closesocket close
#define select win32_select
#define ioctlsocket win32_ioctlsocket


#endif //TRUNK_WS2TCPIP_H
