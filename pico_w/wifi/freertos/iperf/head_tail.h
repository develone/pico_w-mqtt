#ifndef HEAD_TAIL_H
#define HEAD_TAIL_H
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
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

#endif