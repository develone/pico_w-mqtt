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

//static mqtt_client_t* saved_mqtt_client = NULL;
static u8_t mqtt_connected = 1;
static u8_t check_mqtt_connected;

static u8_t check_wifi_connected;
static u8_t wifi_connected = 1;


#define debug_level 

#define TCP_PORT 4001
#define DEBUG_printf printf
#define BUF_SIZE 384
#define TEST_ITERATIONS 1
#define POLL_TIME_S 5
char client_message[BUF_SIZE];
typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    uint8_t buffer_sent[BUF_SIZE];
    uint8_t buffer_recv[BUF_SIZE];
    int sent_len;
    int recv_len;
    int run_count;
} TCP_SERVER_T;

static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

static err_t tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    return err;
}

static err_t tcp_server_result(void *arg, int status) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (status == 0) {
        DEBUG_printf("test success\n");
		//sprintf(tmp,"test success\n");
		//head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);
    } else {
        DEBUG_printf("test failed %d\n", status);
		//sprintf(tmp,"test success\n");
		//head = head_tail_helper(head, tail, endofbuf, topofbuf, tmp);

    }
    state->complete = true;
    return tcp_server_close(arg);
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("tcp_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {

        // We should get the data back from the client
        state->recv_len = 0;
        DEBUG_printf("Waiting for buffer from client\n");
    }

    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    for(int i=0; i< BUF_SIZE; i++) {
        state->buffer_sent[i] = client_message[i];
    }

    state->sent_len = 0;
    DEBUG_printf("Writing %ld bytes to client\n", BUF_SIZE);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    //cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_sent, BUF_SIZE, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return tcp_server_result(arg, -1);
    }
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        return tcp_server_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    //cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        DEBUG_printf("tcp_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);
        DEBUG_printf("RPi sent  %s\n",state->buffer_recv);
        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    // Have we have received the whole buffer
    if (state->recv_len == BUF_SIZE) {

        // check it matches
        //if (memcmp(state->buffer_sent, state->buffer_recv, BUF_SIZE) != 0) {
            //DEBUG_printf("buffer mismatch\n");
            //return tcp_server_result(arg, -1);
        //}
        DEBUG_printf("tcp_server_recv buffer ok\n");

        // Test complete?
        state->run_count++;
        if (state->run_count >= TEST_ITERATIONS) {
            tcp_server_result(arg, 0);
            return ERR_OK;
        }

        // Send another buffer
        return tcp_server_send_data(arg, state->client_pcb);
    }
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_server_poll_fn\n");
    return tcp_server_result(arg, -1); // no response is an error?
}

static void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_server_result(arg, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        tcp_server_result(arg, err);
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return tcp_server_send_data(arg, state->client_pcb);
}

static bool tcp_server_open(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %u\n", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

 /**************************tcp_server**************************/

/**************************head-tail**************************/

char tmp[80];
char * head;
char * tail;
char * endofbuf;
char * topofbuf;

char * bump_head(char * head, char * endofbuf,char * topofbuf);
char * bump_tail(char * tail,char * endofbuf,char * topofbuf);
char * dec_head(char * head,char * endofbuf,char * topofbuf);
char * dec_tail(char * tail,char * endofbuf,char * topofbuf);
char * head_tail_helper(char * head, char * tail,char * endofbuf,char * topofbuf, char * inpstr);

char * head_tail_helper(char * head, char * tail, char * endofbuf,char * topofbuf, char * inpstr) {
	
	u8_t loop = 0;
	u8_t tstlen = 0;
	//printf("0x%x 0x%x 0x%x 0x%x \n", head, tail, endofbuf, topofbuf);
	//printf("%s\n", inpstr);
	tstlen = strlen(inpstr);
	// printf("%d %d \n", loop,tstlen);
	
	for(loop=0;loop< tstlen; loop++) {
		*head = inpstr[loop];
		// printf("head 0x%x head 0x%x E 0x%x  T 0x%x \n", *head,head, endofbuf, topofbuf);
		
		head = bump_head(head, endofbuf, topofbuf);
		// printf("head 0x%x head 0x%x E 0x%x  T 0x%x \n", *head,head, endofbuf, topofbuf);
		
	}
	// printf("head 0x%x head 0x%x E 0x%x  T 0x%x \n", *head,head, endofbuf, topofbuf);
	
	return ((char *)head);
}
 
char * bump_head(char * head, char * endofbuf,char * topofbuf) {
 
	if(head == endofbuf) {

		
			// printf("head == endofbuf\n");
			
			head = topofbuf;
	}
	else {
		// printf("head < endofbuf\n");
		
		head = head + 1;
	}
 
	
	return((char *)head);
}
char * bump_tail(char * tail,char * endofbuf,char * topofbuf) {
	
	if(tail == endofbuf) {

		
			// printf("tail == endofbuf\n");
			
			tail = topofbuf;
	}
	else {
		// printf("tail < endofbuf\n");
		
		tail = tail + 1;
	}
 
	
	return((char *)tail);
}
char * dec_head(char * head,char * endofbuf,char * topofbuf) {
	if(head == topofbuf) {
			// printf("head == topofbuf\n");
			
			//head = topofbuf;
	}
	else {
		//printf("head < topofbuf\n");
		head = head - 1;
	}

	return((char *)head);
}
char * dec_tail(char * tail,char * endofbuf,char * topofbuf) {
	if(tail == topofbuf) {
			printf("tail == topofbuf\n");
			head = topofbuf;
	}
	else {
		printf("tail < topofbuf\n");
		tail = tail - 1;
	}

	return((char *)tail); 
}


/**************************head-tail**************************/


#ifdef __cplusplus
}
#endif

#endif /* MQTT_EXTRA_H */
