#ifndef MQTT_EXTRA_H
#define MQTT_EXTRA_H

#ifdef __cplusplus
extern "C" {
#endif
#include "hardware/watchdog.h"
#include "pico/time.h"
//void mqtt_example_init(void);
/**************************ping**************************/
//#include "ping.h"

/**
 * PING_DEBUG: Enable debugging for PING.
 */
//#ifndef PING_DEBUG
//#define PING_DEBUG     LWIP_DBG_ON
//#endif
//#ifndef PING_ADDR
//#define PING_ADDR "142.251.35.196"
//#endif
/**************************ping**************************/

/**************************tcp_server**************************/

#include <string.h>
#include <stdlib.h>

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

// mqtt_client_t* saved_mqtt_client = NULL;
 u8_t mqtt_connected = 1;
 u8_t check_mqtt_connected;

 u8_t check_wifi_connected;
 u8_t wifi_connected = 1;

char ver[7] = "0.0.07";
#define debug_level 






#ifdef __cplusplus
}
#endif

#endif /* MQTT_EXTRA_H */
