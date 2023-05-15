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
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#ifndef RUN_FREERTOS_ON_CORE
	#define RUN_FREERTOS_ON_CORE 0
#endif
char remotes[6][8]={"remote1","remote2","remote3","remote4","remote5","remote6"};
 
int rr[6];
#define GPIO_TASK_PRIORITY				( tskIDLE_PRIORITY + 8UL )
//#define RTC_TASK_PRIORITY			    ( tskIDLE_PRIORITY + 7UL )
#define WATCHDOG_TASK_PRIORITY			( tskIDLE_PRIORITY + 1UL )
#define MQTT_TASK_PRIORITY				( tskIDLE_PRIORITY + 4UL )  
#define SOCKET_TASK_PRIORITY			( tskIDLE_PRIORITY + 6UL )
#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3UL )

#include "lwip/apps/mqtt.h"
#include "mqtt_example.h"
#include "pico/util/datetime.h"
//char rectime[19];
static volatile bool fired = false;
u8_t alarm_flg=0;
static void alarm_callback(void) {
    datetime_t t = {0};
    rtc_get_datetime(&t);
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("Alarm Fired At %s\n", datetime_str);
    sprintf(tmp,"Alarm Fired %s ",datetime_str);
    head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    stdio_flush();
    fired = true;
    alarm_flg=1;
}
 
typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

/*needed for rtc */
datetime_t t;
datetime_t alarm;
datetime_t t_ntp;
datetime_t *pt;
datetime_t *palarm;
datetime_t *pt_ntp;
u8_t rtc_set_flag = 0;
char datetime_buf[256];
char *datetime_str = &datetime_buf[0];
/*needed for rtc */
/*needed for ntp*/
/*needed for GPIO from pico-examples/gpio/hello_7segment/hello_7segment.c
gpio will be an additional freertos task
*/
#define FIRST_GPIO 18
#define BUTTON_GPIO (FIRST_GPIO+7)
int bit2=1;
int bit3=1;
int bit4=1;
int bit5=1;
uint tbits25;
char bits25[2];
u8_t reset_remote=0;
int val = 0;
int loop;
// This array converts a number 0-9 to a bit pattern to send to the GPIOs
int bits[10] = {
        0x3f,  // 0
        0x3e,  // 1
        0x3d,  // 2
        0x3c,  // 3
        0x3b,  // 4
        0x3a,  // 5
        0x39,  // 6
        0x38,  // 7
        0x37,  // 8
        0x36   // 9
};
// This array converts a number 0-9 to a bit pattern to send to the GPIOs
int32_t mask;
 
u8_t lp;
u8_t alarm_hour;
u8_t alarm_min;
u8_t alarm_sec;
u8_t remote_index;
u8_t cmd;
char houralarm[3];
char minalarm[3];
char secalarm[3];

#define NTP_TASK_PRIORITY				( tskIDLE_PRIORITY + 5UL )
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
  0,  /* keep alive */
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
  cyw43_arch_lwip_begin();
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(data);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" data cb: len %d, flags %d\n", client_info->client_id,
          (int)len, (int)flags));
          //Sunday 2 April 1:34:48 2023      got ntp response: 02/04/2023 01:34:47    2023-04-01-19-48-24 -> 2023/04/01 19:48:24
          if (len==10) {
               
              if (data[0]=='1') remote_index=1;
              if (data[0]=='2') remote_index=2;
              if (data[0]=='3') remote_index=3;
              if (data[0]=='4') remote_index=4;
              if (data[0]=='5') remote_index=5;
              if (data[0]=='6') remote_index=6;
              if (data[0]== 'x') remote_index=255;
              //if (remote_index==255) printf("all remotes will act on cmd\n");
              printf("remote%d\n",remote_index);
              if (data[1]=='1') cmd=1;
              if (data[1]=='2') cmd=2;
              if (data[1]=='3') cmd=3;
              if (data[1]=='4') cmd=4;
              if (data[1]=='5') cmd=5;
              if (data[1]=='6') cmd=6;
              if (data[1]=='7') cmd=7;
              if (data[1]=='8') cmd=8;
              if (data[1]=='9') cmd=9;
              if (cmd==1) {
                  strncpy(&houralarm[0],&data[2],2);
                  strncpy(&minalarm[0],&data[4],2);
                  strncpy(&secalarm[0],&data[6],2);
                  //printf("%s %s %s\n",houralarm,minalarm,secalarm);
                  alarm_hour=atoi(&houralarm[0]);
                  alarm_min=atoi(&minalarm[0]);
                  alarm_sec=atoi(&secalarm[0]);
                  
              }
 
              strncpy(&bits25[0],&data[2],1);
              tbits25=atoi(&bits25[0]);
              printf("data[2] %c tbits25 %d \n",data[2],tbits25);
              process_cmd(remote_index, cmd);
 
          }    
  cyw43_arch_lwip_end();
     
}
void process_cmd(u8_t rem, u8_t cc) {
    
    //printf("0x%x 0x%x 0x%x 0x%x \n",&remotes[0], &remotes[1], &remotes[2], &CYW43_HOST_NAME);
    //printf("%s %s %s \n",remotes[0], remotes[1], remotes[2]);
    //printf("0x%x 0x%x 0x%x \n",&remotes[3], &remotes[4], &remotes[5]);
    //printf("%s %s %s %s\n",remotes[3], remotes[4], remotes[5],CYW43_HOST_NAME);
    rr[0] = strcmp(remotes[0],CYW43_HOST_NAME);
    rr[1]= strcmp(remotes[1],CYW43_HOST_NAME);
    rr[2] = strcmp(remotes[2],CYW43_HOST_NAME); 
    rr[3]= strcmp(remotes[3],CYW43_HOST_NAME);
    rr[4] = strcmp(remotes[4],CYW43_HOST_NAME);
    rr[5] = strcmp(remotes[5],CYW43_HOST_NAME);
    
    //printf("%02d %02d %02d\n",alarm_hour,alarm_min,alarm_sec);
    printf("rem %d cc %d %s\n",rem,cc,CYW43_HOST_NAME);
    //printf("old %d %02d %02d %02d\n",rtc_set_flag,palarm->hour, palarm->min, palarm->sec);
    palarm->hour = alarm_hour;
    palarm->min = alarm_min;
    palarm->sec = alarm_sec;
    //printf("%02d %02d %02d\n",palarm->hour,palarm->min, palarm->sec);

    if(cc==1) {     
    if(((rr[0]==0) && (rem == 1)) || (rem==255)) {
         printf("%s executes  rr %d rem %d\n", remotes[0],rr[0],rem);
         printf("all remotes execute\n");
         alarm_flg=0;
         rtc_set_alarm(&alarm, &alarm_callback);
    }
    if(((rr[1]==0) && (rem == 2)) || (rem==255)) {
        printf("%s executes  rr %d rem %d\n", remotes[0],rr[0],rem);
        printf("all remotes execute\n");
        alarm_flg=0;
        rtc_set_alarm(&alarm, &alarm_callback);
    }  
    if(((rr[2]==0) && (rem == 3)) || (rem==255)) {
         printf("%s executes  rr %d rem %d\n", remotes[0],rr[0],rem);
         printf("all remotes execute\n");
         alarm_flg=0;
         rtc_set_alarm(&alarm, &alarm_callback);
    }
    if(((rr[3]==0) && (rem == 4)) || (rem==255)) {
        printf("%s executes  rr %d rem %d\n", remotes[0],rr[0],rem);
        printf("all remotes execute\n");
        alarm_flg=0;
        rtc_set_alarm(&alarm, &alarm_callback);
    }  
    if(((rr[4]==0) && (rem == 5)) || (rem==255)) {
         printf("%s executes  rr %d rem %d\n", remotes[0],rr[0],rem);
         printf("all remotes execute\n");
		 alarm_flg=0;	
         rtc_set_alarm(&alarm, &alarm_callback);
    }
    if(((rr[5]==0) && (rem == 6)) || (rem==255)) {
        printf("%s executes  rr %d rem %d\n", remotes[0],rr[0],rem);
        printf("all remotes execute\n");
        alarm_flg=0;
        rtc_set_alarm(&alarm, &alarm_callback);
    }    
    } /*cmd = 1*/
    if(cc==2) { 
        if(((rr[0]==0) && (rem == 1)) || (rem==255)) {
            if(tbits25==0) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=0;
            }
            if(tbits25==1) {
                bit2=1;
                bit3=0;
                bit4=0;
                bit5=0;
            }
            if(tbits25==2) {
                bit2=0;
                bit3=1;
                bit4=0;
                bit5=0;
            } 
            if(tbits25==3) {
                bit2=0;
                bit3=0;
                bit4=1;
                bit5=0;
            } 
            if(tbits25==4) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=1;
            }  
        } /*remote1*/   
        if(((rr[1]==0) && (rem == 2)) || (rem==255)) {
            if(tbits25==0) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=0;
            }
            if(tbits25==1) {
                bit2=1;
                bit3=0;
                bit4=0;
                bit5=0;
            }
            if(tbits25==2) {
                bit2=0;
                bit3=1;
                bit4=0;
                bit5=0;
            } 
            if(tbits25==3) {
                bit2=0;
                bit3=0;
                bit4=1;
                bit5=0;
            } 
            else if(tbits25==4) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=1;
            }   
        } /*remote2*/
        if(((rr[2]==0) && (rem == 3)) || (rem==255)) {
            if(tbits25==0) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=0;
            }
            if(tbits25==1) {
                bit2=1;
                bit3=0;
                bit4=0;
                bit5=0;
            }
            if(tbits25==2) {
                bit2=0;
                bit3=1;
                bit4=0;
                bit5=0;
            } 
            if(tbits25==3) {
                bit2=0;
                bit3=0;
                bit4=1;
                bit5=0;
            } 
            if(tbits25==4) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=1;
            }   
        } /*remote3*/
        if(((rr[3]==0) && (rem == 4)) || (rem==255)) {
            if(tbits25==0) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=0;
            }
            if(tbits25==1) {
                bit2=1;
                bit3=0;
                bit4=0;
                bit5=0;
            }
            if(tbits25==2) {
                bit2=0;
                bit3=1;
                bit4=0;
                bit5=0;
            } 
            else if(tbits25==3) {
                bit2=0;
                bit3=0;
                bit4=1;
                bit5=0;
            } 
            if(tbits25==4) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=1;
            }   
        } /*remote4*/                                  
        if(((rr[4]==0) && (rem == 5)) || (rem==255)) {
            if(tbits25==0) {
                bit2=0;
                bit3=0;
                bit4=0;
                bit5=0;
            }
	    if(tbits25==1) {
		bit2=1;
		bit3=0;
		bit4=0;
		bit5=0;
	    }
	    if(tbits25==2) {
		bit2=0;
		bit3=1;
		bit4=0;
		bit5=0;
	    } 
	    if(tbits25==3) {
		bit2=0;
		bit3=0;
		bit4=1;
		bit5=0;
	    } 
	    if(tbits25==4) {
		bit2=0;
		bit3=0;
		bit4=0;
		bit5=1;
	    }    
    }/*remote5*/
        if(((rr[5]==0) && (rem == 6)) || (rem==255)) {
	    for(loop=0;loop<10;loop++) {
		if(tbits25==loop) {
		    val=loop;
		    //gpio_clr_mask(mask);
		    mask = bits[val] << FIRST_GPIO;
		    sprintf(tmp,"val %d ",val);
		    head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
		}
	    }
    }/*remote6*/
    //printf("cmd2 %d %d %d %d \n",bit2,bit3,bit4,bit5);    
    }/*cmd = 2*/
    if(cc==3) {
	if(((rr[0]==0) && (rem == 1)) || (rem==255)) {
	    reset_remote=1;
	    watchdog_enable(10, 1);
	    while(reset_remote) {
	    }
	}
	if(((rr[1]==0) && (rem == 2)) || (rem==255)) {
	    reset_remote=1;
	    watchdog_enable(10, 1);
	    while(reset_remote) {
	    };
	}
	if(((rr[2]==0) && (rem == 3)) || (rem==255)) {
	    reset_remote=1;
	    watchdog_enable(10, 1);
	    while(reset_remote) {
	    }
	}
	if(((rr[3]==0) && (rem == 4)) || (rem==255)) {
	    reset_remote=1;
	    watchdog_enable(10, 1);
	    while(reset_remote) {
	    }
	}
	if(((rr[4]==0) && (rem == 5)) || (rem==255)) {
	    reset_remote=1;
	    watchdog_enable(10, 1);
	    while(reset_remote) {
	    }
	}
	if(((rr[5]==0) && (rem == 6)) || (rem==255)) {
	    reset_remote=1;
	    watchdog_enable(10, 1);
	    while(reset_remote) {
	    }
	}
	
    }
}

static void
mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
  cyw43_arch_lwip_begin();  
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" publish cb: topic %s, len %d\n", client_info->client_id,
          topic, (int)tot_len));
  cyw43_arch_lwip_end();
}

static void
mqtt_request_cb(void *arg, err_t err)
{
  cyw43_arch_lwip_begin();  
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err));
  cyw43_arch_lwip_end();
}
static void
mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  cyw43_arch_lwip_begin();
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
  cyw43_arch_lwip_end();
  }
}
#endif /*LWIP_TCP*/

void
mqtt_example_init(void)
{
#if LWIP_TCP
  cyw43_arch_lwip_begin();
  mqtt_client = mqtt_client_new();
  //printf("mqtt_client 0x%x &mqtt_client 0x%x \n", mqtt_client,&mqtt_client);	
   
  //printf("mqtt_client 0x%x mqtt_client 0x%x \n", mqtt_client,mqtt_client);
  mqtt_set_inpub_callback(mqtt_client,
          mqtt_incoming_publish_cb,
          mqtt_incoming_data_cb,
          LWIP_CONST_CAST(void*, &mqtt_client_info));
  //printf("mqtt_set_inpub_callback 0x%x\n",mqtt_set_inpub_callback);
  

  mqtt_connected = mqtt_client_connect(mqtt_client,
          &mqtt_ip, mqtt_port,
          mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
          &mqtt_client_info);
  cyw43_arch_lwip_end();
  //printf("mqtt_client_connect 0x%x\n",mqtt_client_connect);

 
  //printf("0x%x \n",LWIP_CONST_CAST(void*, &mqtt_client_info));
/*
  strcpy(PUB_PAYLOAD_SCR,PUB_PAYLOAD);
  strcat( PUB_PAYLOAD_SCR,CYW43_HOST_NAME);
  payload_size = sizeof(PUB_PAYLOAD_SCR) + 7;
  printf("%s  %d \n",PUB_PAYLOAD_SCR,sizeof(PUB_PAYLOAD_SCR));
  mqtt_publish(mqtt_client,"pico/status",PUB_PAYLOAD_SCR,payload_size,2,0,pub_mqtt_request_cb_t,PUB_EXTRA_ARG);
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

    //printf("Completed iperf transfer of %d MBytes @ %.1f Mbits/sec\n", mbytes, mbits);
    //printf("Total iperf megabytes since start %d Mbytes\n", total_iperf_megabytes);
#if CYW43_USE_STATS
    //printf("packets in %u packets out %u\n", CYW43_STAT_GET(PACKET_IN_COUNT), CYW43_STAT_GET(PACKET_OUT_COUNT));
#endif
}

/*needed for ntp*/
void ntp_task(__unused void *params) {
    //bool on = false;
    //printf("ntp_task starts\n");
	run_ntp_test();
    while (true) {
#if 0 && configNUM_CORES > 1
        static int last_core_id;
        if (portGET_CORE_ID() != last_core_id) {
            last_core_id = portGET_CORE_ID();
            printf("ntp now from core %d\n", last_core_id);
        }
#endif
        //cyw43_arch_gpio_put(0, on);
        //on = !on;
        
        vTaskDelay(200);
    }
}
/*needed for ntp*/

void watchdog_task(__unused void *params) {
    //bool on = false;

    while (true) {
	 
	//if (wifi_connected == 0) watchdog_update();
	watchdog_update();
    
 
       vTaskDelay(200);
    }
}
void gpio_a_f(void) {
    gpio_set_mask(mask);    
}

void gpio_task(__unused void *params) {
    //bool on = false;
    //printf("gpio_task starts\n");
     
 
        
//We could use gpio_set_dir_out_masked() here



 
    while (true) {
        
        printf("mask %d\n",mask);
        gpio_set_mask(mask);
        //gpio_put(FIRST_GPIO+1,bit3);
        //gpio_put(FIRST_GPIO+2,bit4);
        //gpio_put(FIRST_GPIO+3,bit5);
        //gpio_put(FIRST_GPIO+3,bit5);
        //if(cmd==2) printf("%01d %01d %01d %d01 %01d\n",bit2,bit3,bit4,bit5);
        //printf("%01d %01d %01d %d01 \n",bit2,bit3,bit4,bit5);
        
        vTaskDelay(800);
    }
}

void mqtt_task(__unused void *params) {
    //bool on = false;
    //printf("mqtt_task starts\n");
    cyw43_arch_lwip_begin();
mqtt_subscribe(mqtt_client,"pico/cmds", 2,pub_mqtt_request_cb_t,PUB_EXTRA_ARG);
cyw43_arch_lwip_end();
    while (true) {
        cyw43_arch_lwip_begin();
            check_mqtt_connected = mqtt_client_is_connected(mqtt_client);
            if (check_mqtt_connected==0) printf("mqtt_connection_lost\n");
                mqtt_connection_lost();
        cyw43_arch_lwip_end();
#if 0 && configNUM_CORES > 1
        static int last_core_id;
        if (portGET_CORE_ID() != last_core_id) {
            last_core_id = portGET_CORE_ID();
            //printf("mqtt now from core %d\n", last_core_id);
        }
#endif
        //cyw43_arch_gpio_put(0, on);
        //on = !on;
        //printf("in mqtt\n");
  strcpy(PUB_PAYLOAD_SCR,PUB_PAYLOAD);
  strcat( PUB_PAYLOAD_SCR,CYW43_HOST_NAME);
  payload_size = sizeof(PUB_PAYLOAD_SCR) + 7;
  //printf("%s  %d \n",PUB_PAYLOAD_SCR,sizeof(PUB_PAYLOAD_SCR));
  //sprintf(tmp,"mqtt_connect 0x%x ",check_mqtt_connected);
  //head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);

  //sprintf(tmp,"mq_con 0x%x ",check_mqtt_connected);
  //head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
  
  	
  /*
  mqtt_client_is_connected 1 if connected to server, 0 otherwise 
  */
  cyw43_arch_lwip_begin();	
  mqtt_publish(mqtt_client,"pico/status",PUB_PAYLOAD_SCR,payload_size,2,0,pub_mqtt_request_cb_t,PUB_EXTRA_ARG);
  cyw43_arch_lwip_end();	
        vTaskDelay(1000);
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
            //printf("blinking now from core %d\n", last_core_id);
        }
#endif
        cyw43_arch_gpio_put(0, on);
        on = !on;

        vTaskDelay(200);
    }
}

void main_task(__unused void *params) {
    if (cyw43_arch_init()) {
        //printf("failed to initialise\n");
        return;
    }
	watchdog_enable(10000, 1);
	//while (wifi_connected) {
    	cyw43_arch_enable_sta_mode();
    	//printf("Connecting to Wi-Fi...\n");
		//sprintf(tmp,"Connecting to Wi-Fi...");
		//head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        	//printf("failed to connect.\n");
		init_pico_mqtt();
        	exit(1);
    	} else {
        	
    		 
    	}
	//}	 
	init_pico_mqtt();
    while(true) {
        // not much to do as LED is in another task, and we're using RAW (callback) lwIP API
 
        vTaskDelay(10000);
    }

    cyw43_arch_deinit();
}

void mqtt_connection_lost(void) {
    if (check_mqtt_connected==0) printf("mqtt_connection_lost\n");
}

void init_pico_mqtt(void) {
    printf("Connected.\n");
    //printf("%d %d %d %d\n",bit2,bit3,bit4,bit5);
    sprintf(tmp,"Connected. iperf server %s %u  ",ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
    head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    //sprintf(tmp,"starting watchdog timer task ")
    //head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    //printf("mqtt_ip = 0x%x &mqtt_ip = 0x%x\n",mqtt_ip,&mqtt_ip);
    //printf("mqtt_port = %d &mqtt_port 0x%x\n",mqtt_port,&mqtt_port);
    sprintf(tmp,"mqtt_ip = 0x%x mqtt_port = %d  ",mqtt_ip,mqtt_port);
    head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    //topofbuf = (char *)&client_message[256];
    for (int gpio = FIRST_GPIO; gpio < FIRST_GPIO + 4; gpio++) {
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_OUT);
        gpio_set_outover(gpio, GPIO_OVERRIDE_INVERT);
    }
     
     

    mqtt_example_init();
    sleep_ms(1000);
    //wifi_connected = 0;
    xTaskCreate(watchdog_task, "WatchdogThread", configMINIMAL_STACK_SIZE, NULL, WATCHDOG_TASK_PRIORITY, NULL);
    xTaskCreate(blink_task, "BlinkThread", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL);
    xTaskCreate(socket_task, "SOCKETThread", configMINIMAL_STACK_SIZE, NULL, SOCKET_TASK_PRIORITY, NULL);
	xTaskCreate(mqtt_task, "MQTTThread", configMINIMAL_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, NULL);
    //xTaskCreate(rtc_task, "RTCThread", configMINIMAL_STACK_SIZE, NULL, RTC_TASK_PRIORITY, NULL);
    xTaskCreate(ntp_task, "NTPThread", configMINIMAL_STACK_SIZE, NULL, NTP_TASK_PRIORITY, NULL);
    xTaskCreate(gpio_task, "GPIOThread", configMINIMAL_STACK_SIZE, NULL, GPIO_TASK_PRIORITY, NULL);

    cyw43_arch_lwip_begin();
#if CLIENT_TEST
    //printf("\nReady, running iperf client\n");
    ip_addr_t clientaddr;
    ip4_addr_set_u32(&clientaddr, ipaddr_addr(xstr(IPERF_SERVER_IP)));
    assert(lwiperf_start_tcp_client_default(&clientaddr, &iperf_report, NULL) != NULL);
#else
    //printf("\nReady, running iperf server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    lwiperf_start_tcp_server_default(&iperf_report, NULL);
#endif
    cyw43_arch_lwip_end();

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
void set_rtc(datetime_t *pt, datetime_t *pt_ntp,datetime_t *palarm) {
	if(rtc_set_flag==0) {
		pt->year = pt_ntp->year;
		pt->month = pt_ntp->month; 
		pt->day = pt_ntp->day;
		//pt->dotw = 0;
		pt->hour = pt_ntp->hour;
		pt->min = pt_ntp->min;
		pt->sec = pt_ntp->sec;
		palarm->year = pt_ntp->year;
		palarm->month = pt_ntp->month;
		palarm->day = pt_ntp->day;
		//palarm->dotw = 0;
		palarm->hour = pt_ntp->hour;
		palarm->min = pt_ntp->min + 1;
		palarm->sec = pt_ntp->sec;
		rtc_set_flag=1;
    	// Start the RTC
    	rtc_init();
    	rtc_set_datetime(&t);
		sleep_us(64);
		rtc_set_alarm(&alarm, &alarm_callback);

	}
        rtc_get_datetime(&t);
        datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
        //printf("\r%s      ", datetime_str);
        //printf("0x%x 0x%x\n",&t,&alarm);
        //printf("pt 0x%x palarm 0x%x\n",pt,palarm);
}
/*needed for ntp*/
// Called with results of operation
static void ntp_result(NTP_T* state, int status, time_t *result) {
    if (status == 0 && result) {
        struct tm *utc = gmtime(result);
		t_ntp.year = utc->tm_year + 1900;
		t_ntp.month = utc->tm_mon + 1; 
		t_ntp.day = utc->tm_mday;
		t_ntp.hour = utc->tm_hour;
		t_ntp.min = utc->tm_min;
		t_ntp.sec = utc->tm_sec;
		pt = &t;
		pt_ntp = &t_ntp;
		palarm = &alarm;
		//printf("0x%x 0x%x 0x%x\n",pt,pt_ntp,palarm);
		set_rtc(pt,pt_ntp,palarm);
		
		//printf("%02d:%02d:%02d\n",pt->hour,pt->min,pt->sec); 
        printf("got ntp response: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
               utc->tm_hour, utc->tm_min, utc->tm_sec);
               sprintf(tmp," %02d/%02d/%04d %02d:%02d:%02d ", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
               utc->tm_hour, utc->tm_min, utc->tm_sec);
               head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
               //topofbuf = (char *)&client_message[256];
               
    }

    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }
    state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
    state->dns_request_sent = false;
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

// Make an NTP request
static void ntp_request(NTP_T *state) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *) p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data)
{
    NTP_T* state = (NTP_T*)user_data;
    printf("ntp request failed\n");
    ntp_result(state, -1, NULL);
	wifi_connected=1;
    return 0;
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T*)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("ntp address %s\n", ip4addr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        printf("ntp dns request failed\n");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_T *state = (NTP_T*)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        ntp_result(state, 0, &epoch);
    } else {
        printf("invalid ntp response\n");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
static NTP_T* ntp_init(void) {
    NTP_T *state = calloc(1, sizeof(NTP_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        printf("failed to create pcb\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}

// Runs ntp test forever
void run_ntp_test(void) {
    NTP_T *state = ntp_init();
    if (!state)
        return;
    while(true) {
        if (absolute_time_diff_us(get_absolute_time(), state->ntp_test_time) < 0 && !state->dns_request_sent) {

            // Set alarm in case udp requests are lost
            state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

            // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
            // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
            // these calls are a no-op and can be omitted, but it is a good practice to use them in
            // case you switch the cyw43_arch type later.
            cyw43_arch_lwip_begin();
            int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
            cyw43_arch_lwip_end();

            state->dns_request_sent = true;
            if (err == ERR_OK) {
                ntp_request(state); // Cached result
            } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
                printf("dns request failed\n");
                ntp_result(state, -1, NULL);
            }
        }
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for WiFi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        sleep_ms(1);
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }
    free(state);
}
/*needed for ntp*/

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
	sprintf(tmp,"Starting %s on core 0: ver %s %s ", rtos_name,ver,CYW43_HOST_NAME);
	head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    vLaunch();
#endif
    return 0;
}

 
