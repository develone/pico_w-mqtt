#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifdef rem1
 
	#define ip "192.168.1.176"
#endif
#ifdef rem2
 
	#define ip "192.168.1.160"
#endif
#ifdef rem3
 
	#define ip "192.168.1.178"
#endif
#ifdef rem4
 
	#define ip "192.168.1.177"
#endif
#ifdef rem5
 
	#define ip "192.168.1.159"
#endif
#ifdef rem6
 
	#define ip "192.168.1.175"
#endif


int main(void)
{
    int socket_desc,flag=1,userflg,msg,buff_size=256;
    struct sockaddr_in server_addr;
    char server_message[buff_size], client_message[buff_size];
    
    // Clean buffers:
    memset(server_message,'\0',sizeof(server_message));
    memset(client_message,'\0',sizeof(client_message));
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Unable to create socket\n");
        return -1;
    }
    
    printf("Socket created successfully\n");
    
    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4001);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    
    // Send connection request to server:
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");
	recv(socket_desc, server_message, buff_size, 0);
    userflg=1;
    msg=1;
    while(flag) {
        //if(msg==4) msg=1;
        if (userflg==0) {
            
            // Get input from the user:
            printf("Enter message: ");
            gets(client_message);
        }
        else {
            printf("%d\n",msg);
            
 
                if(msg==1) sprintf(client_message,"0123456789012345678901234567890012345678901234567890123456789012");
                if(msg==2) sprintf(client_message,"0123456789012345678901234567890012345678901234567890123456789012");
                if(msg==3) sprintf(client_message,"0123456789012345678901234567890012345678901234567890123456789012");
                if(msg==4) sprintf(client_message,"0123456789012345678901234567890012345678901234567890123456789012");

            msg= msg + 1;
            if(msg==5) msg=1;
            sleep(1);
        }
        // Send the message to server:
        if(send(socket_desc, client_message, strlen(client_message), 0) < 0){
            printf("Unable to send message\n");
            return -1;
        }
    
        // Receive the server's response:
        /*if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
            printf("Error while receiving server's msg\n");
            return -1;
        }*/
    
        printf("Server's response: %s\n",server_message);
    }
    
    // Close the socket:
    close(socket_desc);
    
    return 0;
}
