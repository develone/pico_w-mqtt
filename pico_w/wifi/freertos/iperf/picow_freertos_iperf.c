/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "tcp_debug.h"
#include "head_tail.h"
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
#define RTC_TASK_PRIORITY			    ( tskIDLE_PRIORITY + 7UL )
#define WATCHDOG_TASK_PRIORITY			( tskIDLE_PRIORITY + 1UL )
#define MQTT_TASK_PRIORITY				( tskIDLE_PRIORITY + 4UL )  
#define SOCKET_TASK_PRIORITY			( tskIDLE_PRIORITY + 6UL )
#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3UL )

#include "lwip/apps/mqtt.h"
#include "mqtt_example.h"
#include "pico/util/datetime.h"
char rectime[19];
static volatile bool fired = false;
static void alarm_callback(void) {
    datetime_t t = {0};
    rtc_get_datetime(&t);
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("Alarm Fired At %s\n", datetime_str);
    stdio_flush();
    fired = true;
}
/*needed for rtc */
datetime_t t;
datetime_t alarm;
datetime_t t_ntp;
datetime_t *pt;
datetime_t *palarm;
datetime_t *pt_ntp;
u8_t rtc_set_flag = 0;
char datetime_buf[256];
char tmp1[5],tmp2[3];
char *datetime_str = &datetime_buf[0];


mqtt_request_cb_t pub_mqtt_request_cb_t; 
  
u16_t mqtt_port = 1883;
 
#if LWIP_TCP /*LWIP_TCP*/

	/** Define this to a compile-time IP address initialization
	 * to connect anything else than IPv4 loopback
	 */
	#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
	#if LWIP_IPV4 /*LWIP_IPV4*/

			/*192.168.1.212 0xc0a801d4 LWIP_MQTT_EXAMPLE_IPADDR_INIT pi4-50*/
			#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(PP_HTONL(0xc0a801d4))

	#else
			#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
	#endif
	#endif

char PUB_PAYLOAD[] = "this is a message from pico_w ctrl 0       ";
char PUB_PAYLOAD_SCR[] = "this is a message from pico_w ctrl 0       ";
char PUB_EXTRA_ARG[] = "test";
u16_t payload_size;

static ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
static mqtt_client_t* mqtt_client;
 
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
          //Sunday 2 April 1:34:48 2023      got ntp response: 02/04/2023 01:34:47    2023-04-01-19-48-24 -> 2023/04/01 19:48:24

  if (len==19) {
      strncpy(rectime,data,19);
      sprintf(tmp, "%s \n",rectime);
      head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
      if(rtc_set_flag==0) {
          printf("t 0x%x &t 0x%x *pt 0x%x  \n",t,&t,*pt );
          printf("t_ntp 0x%x &pt_ntp 0x%x *pt_ntp 0x%x  \n",t_ntp,&t_ntp,*pt_ntp );
          
          strncpy(tmp1,&data[0],4);
          t.year = atoi(tmp1);
          printf("%d\n",t.year);
          strncpy(tmp2,&data[5],2);
          t.month = atoi(tmp2);
          printf("%02d\n",t.month);
          strncpy(tmp2,&data[8],2);
          t.day = atoi(tmp2);
          printf("%02d\n",t.day);
          strncpy(tmp2,&data[11],2);
          t.hour = atoi(tmp2);
          printf("%02d\n",t.hour);
          strncpy(tmp2,&data[14],2);
          t.min = atoi(tmp2);
          printf("%02d\n",t.min);
          strncpy(tmp2,&data[17],2);
          t.sec = atoi(tmp2);
          printf("%02d\n",t.sec);
          rtc_set_flag=1;
          rtc_init();
          rtc_set_datetime(&t);
          sleep_us(64);
          
      }
      printf("%s \n",rectime);


  }
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
static void
mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" connection cb: status %d\n", client_info->client_id, (int)status));

  if (status == MQTT_CONNECT_ACCEPTED) {
    mqtt_sub_unsub(client,
            "topic_qos1", 1,
            mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
            1);
    mqtt_sub_unsub(client,
            "topic_qos0", 0,
            mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
            1);
  }
}
#endif /*LWIP_TCP*/

void
mqtt_example_init(void)
{
#if LWIP_TCP
  mqtt_client = mqtt_client_new();
  printf("mqtt_client 0x%x &mqtt_client 0x%x \n", mqtt_client,&mqtt_client);	
   
  printf("mqtt_client 0x%x mqtt_client 0x%x \n", mqtt_client,mqtt_client);
  mqtt_set_inpub_callback(mqtt_client,
          mqtt_incoming_publish_cb,
          mqtt_incoming_data_cb,
          LWIP_CONST_CAST(void*, &mqtt_client_info));
  printf("mqtt_set_inpub_callback 0x%x\n",mqtt_set_inpub_callback);
  

  mqtt_connected = mqtt_client_connect(mqtt_client,
          &mqtt_ip, mqtt_port,
          mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
          &mqtt_client_info);
  printf("mqtt_client_connect 0x%x\n",mqtt_client_connect);

 
  //printf("0x%x \n",LWIP_CONST_CAST(void*, &mqtt_client_info));
/*
  strcpy(PUB_PAYLOAD_SCR,PUB_PAYLOAD);
  strcat( PUB_PAYLOAD_SCR,CYW43_HOST_NAME);
  payload_size = sizeof(PUB_PAYLOAD_SCR) + 7;
  printf("%s  %d \n",PUB_PAYLOAD_SCR,sizeof(PUB_PAYLOAD_SCR));
  mqtt_publish(mqtt_client,"update/memo",PUB_PAYLOAD_SCR,payload_size,2,0,pub_mqtt_request_cb_t,PUB_EXTRA_ARG);
*/   
          
#endif /* LWIP_TCP */
}

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

void rtc_task(__unused void *params) {
   
    while (true) {
        if(rtc_set_flag==1) {
            rtc_get_datetime(&t);
            printf("%04d/%02d/%02d %02d:%02d:%02d\n",t.year,t.month,t.day,t.hour,t.min,t.sec);
        }
 
 
        
            
        vTaskDelay(25000);
    }
}


void watchdog_task(__unused void *params) {
    //bool on = false;

    while (true) {
	 
	//if (wifi_connected == 0) watchdog_update();
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
  sprintf(tmp,"mqtt_connect 0x%x ",check_mqtt_connected);
  head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
  check_mqtt_connected = mqtt_client_is_connected(mqtt_client);
  sprintf(tmp,"mqtt_connect 0x%x\n",check_mqtt_connected);
  head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
  if (check_mqtt_connected == 0) {
    printf("in re-connect\n");
    mqtt_connected = 1;
    sprintf(tmp,"in re-connect forceing watcdof rebiit %d\n",mqtt_connected);
    head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    mqtt_disconnect(mqtt_client);
   // mqtt_client_free(mqtt_client);
   watchdog_enable(100, 1);
  
    mqtt_example_init();
    sleep_ms(1000);
  }
  	
  /*
  mqtt_client_is_connected 1 if connected to server, 0 otherwise 
  */
	
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
	watchdog_enable(10000, 1);
	//while (wifi_connected) {
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
			topofbuf = (char *)&client_message[256];
			mqtt_example_init();
            sleep_ms(1000);
			//wifi_connected = 0;
    		 
    	}
	//}	 
	

	xTaskCreate(watchdog_task, "WatchdogThread", configMINIMAL_STACK_SIZE, NULL, WATCHDOG_TASK_PRIORITY, NULL);
    xTaskCreate(blink_task, "BlinkThread", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL);
    xTaskCreate(socket_task, "SOCKETThread", configMINIMAL_STACK_SIZE, NULL, SOCKET_TASK_PRIORITY, NULL);
	xTaskCreate(mqtt_task, "MQTTThread", configMINIMAL_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, NULL);
    xTaskCreate(rtc_task, "RTCThread", configMINIMAL_STACK_SIZE, NULL, RTC_TASK_PRIORITY, NULL);

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
0123456789012345678901234567890123456789012345678901234567890123\
0123456789012345678901234567890123456789012345678901234567890123\
0123456789012345678901234567890123456789012345678901234567890123\
0123456789012345678901234567890123456789012345678901234567890123\
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
	sprintf(tmp,"Starting %s on core 0: ver %s %s\n", rtos_name,ver,CYW43_HOST_NAME);
	head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    vLaunch();
#endif
    return 0;
}

 
