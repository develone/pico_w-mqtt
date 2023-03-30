#include "head_tail.h"
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

