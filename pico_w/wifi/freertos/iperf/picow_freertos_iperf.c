/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mqtt_extra.h"

 
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <string.h>
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps/lwiperf.h"

#include "FreeRTOS.h"
#include "task.h"
#ifndef RUN_FREERTOS_ON_CORE
	#define RUN_FREERTOS_ON_CORE 0
#endif
#include "mqtt_example.h"
#include "lwip/apps/mqtt.h"
#include "mqtt_example.h"
mqtt_request_cb_t pub_mqtt_request_cb_t;  
u16_t mqtt_port = 1883;
 
#if LWIP_TCP

	/** Define this to a compile-time IP address initialization
	 * to connect anything else than IPv4 loopback
	 */
	#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
		#if LWIP_IPV4

			/*192.168.1.212 0xc0a801d4 LWIP_MQTT_EXAMPLE_IPADDR_INIT pi4-50*/
			#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(PP_HTONL(0xc0a801d4))

		#else
			#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
		#endif
	#endif
#endif

char PUB_PAYLOAD[] = "this is a message from pico_w ctrl 0       ";
char PUB_PAYLOAD_SCR[] = "this is a message from pico_w ctrl 0       ";
char PUB_EXTRA_ARG[] = "test";
u16_t payload_size;
static mqtt_client_t* mqtt_client;
static mqtt_client_t* saved_mqtt_client = NULL;
static ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
//typedef void(* 	mqtt_connection_cb_t) (mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
//mqtt_request_cb_t pub_mqtt_request_cb_t; 
static const struct mqtt_connect_client_info_t mqtt_client_info =
{
  CYW43_HOST_NAME,
  "testuser", /* user */
  "password123", /* pass */
  200,  /* keep alive */
  "topic_qos0", /* will_topic */
  NULL, /* will_msg */
  0,    /* will_qos */
  0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};

static void
mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(data);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" data cb: len %d, flags %d\n", client_info->client_id,
          (int)len, (int)flags));
  if (len==19) printf("%s \n",data);
}

static void
mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" publish cb: topic %s, len %d\n", client_info->client_id,
          topic, (int)tot_len));
}

static void
mqtt_request_cb(void *arg, err_t err)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err));
}

#define WATCHDOG_TASK_PRIORITY			( tskIDLE_PRIORITY + 6UL )
#define MQTT_TASK_PRIORITY				( tskIDLE_PRIORITY + 4UL )  
#define SOCKET_TASK_PRIORITY			( tskIDLE_PRIORITY + 3UL )
#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY				( tskIDLE_PRIORITY + 1UL )

#if CLIENT_TEST && !defined(IPERF_SERVER_IP)
#error IPERF_SERVER_IP not defined
#endif

// Report IP results and exit
static void iperf_report(void *arg, enum lwiperf_report_type report_type,
                         const ip_addr_t *local_addr, u16_t local_port, const ip_addr_t *remote_addr, u16_t remote_port,
                         u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec) {
    static uint32_t total_iperf_megabytes = 0;
    uint32_t mbytes = bytes_transferred / 1024 / 1024;
    float mbits = bandwidth_kbitpsec / 1000.0;

    total_iperf_megabytes += mbytes;

    printf("Completed iperf transfer of %d MBytes @ %.1f Mbits/sec\n", mbytes, mbits);
    printf("Total iperf megabytes since start %d Mbytes\n", total_iperf_megabytes);
}


void watchdog_task(__unused void *params) {
    //bool on = false;
    printf("watchdog_task starts\n");
    watchdog_enable(10000, 1);
    while (true) {
	 
	watchdog_update();
 
       vTaskDelay(200);
    }
}

void mqtt_task(__unused void *params) {
    //bool on = false;
    printf("mqtt_task starts\n");
mqtt_subscribe(mqtt_client,"pub_time", 2,pub_mqtt_request_cb_t,PUB_EXTRA_ARG);

    while (true) {
#if 0 && configNUM_CORES > 1
        static int last_core_id;
        if (portGET_CORE_ID() != last_core_id) {
            last_core_id = portGET_CORE_ID();
            printf("mqtt now from core %d\n", last_core_id);
        }
#endif
        //cyw43_arch_gpio_put(0, on);
        //on = !on;
        printf("in mqtt\n");
  strcpy(PUB_PAYLOAD_SCR,PUB_PAYLOAD);
  strcat( PUB_PAYLOAD_SCR,CYW43_HOST_NAME);
  payload_size = sizeof(PUB_PAYLOAD_SCR) + 7;
  printf("%s  %d \n",PUB_PAYLOAD_SCR,sizeof(PUB_PAYLOAD_SCR));
  check_mqtt_connected = mqtt_client_is_connected(saved_mqtt_client);
  if (check_mqtt_connected == 0) {
	mqtt_client_free(saved_mqtt_client);
	mqtt_example_init();
  } 	
  /*
  mqtt_client_is_connected 1 if connected to server, 0 otherwise 
  */
  printf("saved_mqtt_client 0x%x check_mqtt_connected %d \n", saved_mqtt_client,check_mqtt_connected);

  mqtt_publish(mqtt_client,"update/memo",PUB_PAYLOAD_SCR,payload_size,2,0,pub_mqtt_request_cb_t,PUB_EXTRA_ARG);
	
        vTaskDelay(25000);
    }
}

void socket_task(__unused void *params) {
	
	//printf("socket_task starts\n");
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) {
        return;
    }
    if (!tcp_server_open(state)) {
        tcp_server_result(state, -1);
        return;
    }
    

    while (true) {
		if(state->complete) {
			free(state);
			sleep_ms(1000);
			TCP_SERVER_T *state = tcp_server_init();
    		if (!state) {
        		return;
    		}
    		if (!tcp_server_open(state)) {
        		tcp_server_result(state, -1);
        		return;
         	}
		}

        vTaskDelay(200);
		
    }
}

void blink_task(__unused void *params) {
    bool on = false;
    //printf("blink_task starts\n");
    while (true) {
#if 0 && configNUM_CORES > 1
        static int last_core_id;
        if (portGET_CORE_ID() != last_core_id) {
            last_core_id = portGET_CORE_ID();
            printf("blinking now from core %d\n", last_core_id);
        }
#endif
        cyw43_arch_gpio_put(0, on);
        on = !on;

        vTaskDelay(200);
    }
}

void main_task(__unused void *params) {
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
	sprintf(tmp,"Connecting to Wi-Fi...\n");
	head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        exit(1);
    } else {
        printf("Connected.\n");
 		sprintf(tmp,"Connected. iperf server %s %u \n",ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
		head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
		sprintf(tmp,"starting watchdog timer task\n");
		head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
		printf("mqtt_ip = 0x%x &mqtt_ip = 0x%x\n",mqtt_ip,&mqtt_ip);
		printf("mqtt_port = %d &mqtt_port 0x%x\n",mqtt_port,&mqtt_port);
		sprintf(tmp,"mqtt_ip = 0x%x mqtt_port = %d \n",mqtt_ip,mqtt_port);
		head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    	//ip_addr_t ping_addr;
    	//ipaddr_aton(PING_ADDR, &ping_addr);
    	//ping_init(&ping_addr);
    }
	//run_tcp_server_test();
	//xTaskCreate(mqtt_task, "MQTTThread", configMINIMAL_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, NULL);

	xTaskCreate(watchdog_task, "WatchdogThread", configMINIMAL_STACK_SIZE, NULL, WATCHDOG_TASK_PRIORITY, NULL);
    xTaskCreate(blink_task, "BlinkThread", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL);
    xTaskCreate(socket_task, "SOCKETThread", configMINIMAL_STACK_SIZE, NULL, SOCKET_TASK_PRIORITY, NULL);


    cyw43_arch_lwip_begin();
#if CLIENT_TEST
    printf("\nReady, running iperf client\n");
    ip_addr_t clientaddr;
    ip4_addr_set_u32(&clientaddr, ipaddr_addr(xstr(IPERF_SERVER_IP)));
    assert(lwiperf_start_tcp_client_default(&clientaddr, &iperf_report, NULL) != NULL);
#else
    printf("\nReady, running iperf server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    lwiperf_start_tcp_server_default(&iperf_report, NULL);
#endif
    cyw43_arch_lwip_end();

    while(true) {
        // not much to do as LED is in another task, and we're using RAW (callback) lwIP API
 
        vTaskDelay(10000);
    }

    cyw43_arch_deinit();
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "TestMainThread", configMINIMAL_STACK_SIZE, NULL, TEST_TASK_PRIORITY, &task);
	

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}



void preptopidata() {
sprintf(client_message,"0123456789012345678901234567890123456789012345678901234567890123\
012345678901234567890123456789012345678901234567890123456789012301234567890123456789012345\
6789012345678901234567890123456789012301234567890123456789012345678901234567890123456789012345678901\n");
 
}

int main( void )
{
    stdio_init_all();
	preptopidata();
	
	head = (char *)&client_message[0];
	tail = (char *)&client_message[0];
	topofbuf = (char *)&client_message[0];
	endofbuf = (char *)&client_message[BUF_SIZE-1];
	// printf("0x%x 0x%x 0x%x 0x%x \n", head, tail, endofbuf, topofbuf);
	

    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
#if ( portSUPPORT_SMP == 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( portSUPPORT_SMP == 1 ) && ( configNUM_CORES == 2 )
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif ( RUN_FREERTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
	printf("Starting %s on core 0:\n", rtos_name);
	sprintf(tmp,"Starting %s on core 0: %s\n", rtos_name,CYW43_HOST_NAME);
	head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    vLaunch();
#endif
    return 0;
}
