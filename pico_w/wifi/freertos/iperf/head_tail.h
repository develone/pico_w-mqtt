#ifndef HEAD_TAIL_H
#define HEAD_TAIL_H
 
/*
char * head;
char * tail;
char * endofbuf;
char * topofbuf;
*/

char * bump_head(char * ptrhead, char * ptrendofbuf,char * ptrtopofbuf);
char * bump_tail(char * ptrtail,char * ptrendofbuf,char * ptrtopofbuf);
char * dec_head(char * ptrhead,char * ptrendofbuf,char * ptrtopofbuf);
char * dec_tail(char * ptrtail,char * ptrendofbuf,char * ptrtopofbuf);
char * head_tail_helper(char * ptrhead, char * ptrtail,char * ptrendofbuf,char * ptrtopofbuf, char * inpstr);

#endif
