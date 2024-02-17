#include "head_tail.h"
 
 
  
char * head_tail_helper(char * ptrhead, char * ptrtail,char * ptrendofbuf,char * ptrtopofbuf, char * inpstr) {
	
	int loop = 0;
    int tstlen = 0;
	printf("0x%x 0x%x 0x%x 0x%x \n", ptrhead, ptrtail, ptrendofbuf, ptrtopofbuf);
	printf("%s\n", inpstr);
	tstlen = strlen(inpstr);
	// printf("%d %d \n", loop,tstlen);
	
	for(loop=0;loop< tstlen; loop++) {
		*ptrhead = inpstr[loop];
		// printf("head 0x%x head 0x%x E 0x%x  T 0x%x \n", *head,head, endofbuf, topofbuf);
		
		ptrhead = bump_head(ptrhead, ptrendofbuf, ptrtopofbuf);
		// printf("head 0x%x head 0x%x E 0x%x  T 0x%x \n", *head,head, endofbuf, topofbuf);
		
	}
	// printf("head 0x%x head 0x%x E 0x%x  T 0x%x \n", *head,head, endofbuf, topofbuf);
	
	return ((char *)ptrhead);
}
 
char * bump_head(char * ptrhead, char * ptrendofbuf,char * ptrtopofbuf) {
 
	if(ptrhead == ptrendofbuf) {

		
			// printf("head == endofbuf\n");
			
			ptrhead = ptrtopofbuf;
	}
	else {
		// printf("head < endofbuf\n");
		
		ptrhead = ptrhead + 1;
	}
 
	
	return((char *)ptrhead);
}
char * bump_tail(char * ptrtail,char * ptrendofbuf,char * ptrtopofbuf) {
	
	if(ptrtail == ptrendofbuf) {

		
			// printf("tail == endofbuf\n");
			
			ptrtail = ptrtopofbuf;
	}
	else {
		// printf("tail < endofbuf\n");
		
		ptrtail = ptrtail + 1;
	}
 
	
	return((char *)ptrtail);
}
char * dec_head(char * ptrhead,char * ptrendofbuf,char * ptrtopofbuf) {
	if(ptrhead == ptrtopofbuf) {
			// printf("head == topofbuf\n");
			
			//head = topofbuf;
	}
	else {
		//printf("head < topofbuf\n");
		ptrhead = ptrhead - 1;
	}

	return((char *)ptrhead);
}
char * dec_tail(char * ptrtail,char * ptrendofbuf,char * ptrtopofbuf) {
	if(ptrtail == ptrtopofbuf) {
			//printf("tail == topofbuf\n");
			ptrtail = ptrtopofbuf;
	}
	else {
		//printf("tail < topofbuf\n");
		ptrtail = ptrtail - 1;
	}

	return((char *)ptrtail); 
}

