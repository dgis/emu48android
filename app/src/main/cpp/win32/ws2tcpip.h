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

typedef struct sockaddr_in6 SOCKADDR_IN6_LH;
typedef SOCKADDR_IN6_LH *PSOCKADDR_IN6;
typedef struct in6_addr *PIN6_ADDR;

#define closesocket close

#endif //TRUNK_WS2TCPIP_H
