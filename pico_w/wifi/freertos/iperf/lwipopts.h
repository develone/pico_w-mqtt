#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0
#endif
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1
#else
// MEM_LIBC_MALLOC is incompatible with non polling versions
#define MEM_LIBC_MALLOC             0
#endif
// Generally you would define your own explicit list of lwIP options
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)
//
// This example uses a common include to avoid repetition
#include "lwipopts_examples_common.h"
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

#define 	PBUF_LINK_HLEN   (14 + ETH_PAD_SIZE)
#define 	PBUF_LINK_ENCAPSULATION_HLEN   0
#define 	PBUF_POOL_BUFSIZE   LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)
#define 	LWIP_PBUF_REF_T   u8_t

#define MEMP_NUM_SYS_TIMEOUT   (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 4)
#define MQTT_REQ_MAX_IN_FLIGHT  (5) /* maximum of subscribe requests */
//picow_freertos_iperf_server_sys
//NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
//define No_SYS 0
//#if !NO_SYS
//#define TCPIP_THREAD_STACKSIZE 1024
//#define DEFAULT_THREAD_STACKSIZE 1024
//#define DEFAULT_RAW_RECVMBOX_SIZE 8
//#define TCPIP_MBOX_SIZE 8
//#define LWIP_TIMEVAL_PRIVATE 0
//#else 
#define ICMP_DEBUG                  LWIP_DBG_ON
#define NETIF_DEBUG                 LWIP_DBG_ON
#define MEM_DEBUG                   LWIP_DBG_ON
#define MEMP_DEBUG                  LWIP_DBG_ON
#define UDP_DEBUG                   LWIP_DBG_ON
#define IP_DEBUG                    LWIP_DBG_ON
#define PBUF_DEBUG                  LWIP_DBG_ON
#define TCPIP_DEBUG                 LWIP_DBG_ON
// not necessary, can be done either way
#define LWIP_TCPIP_CORE_LOCKING_INPUT 1

// ping_thread sets socket receive timeout, so enable this feature
#define LWIP_SO_RCVTIMEO 1
//#endif


#endif
