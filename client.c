#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#define MAX_MESSAGE_LEN 100
#define PORT "10000" // Client 所要連線的 port

int s_fd;

/*void delay(int amount){
	while(amount--){;}
	
}*/

/*const char* rand_str(char *rand_string, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *rand_string++ = charset[index];
    }
    *rand_string = '\0';
	return rand_string ;
}*/

void delay(int sec) { 
	int start=clock(); 
	int end; 
	while(1) { 
		end=clock(); 
		if(((end-start)/1000.)==sec) 
		return; 
	}	 
} 


void close_socket(){
	/***
	Close socket.
	***/
	puts("Closing socket...");
	close(s_fd);
	puts("Socket closed");
	close(s_fd);
	puts("-----------------------------------------------------");
}

/***
Thread: Receive message from server and response.
***/
void *receive(void *argv){
	char recvMsg[MAX_MESSAGE_LEN + 1];
	memset(recvMsg, 0, MAX_MESSAGE_LEN + 1);
	while(1) {	

		
	    
		//printf("Receiving message from server...\n");

		if(recv(s_fd, recvMsg, MAX_MESSAGE_LEN, 0) == -1) {
			puts("Fail to receive message from server.");
			exit(2);
		} else {
			//if(strcmp(recvMsg,"close")==0) close_socket();
			//else{
				printf("%s", recvMsg);
				//printf("\nSomebody said:  %s", recvMsg);
				//printf("\n\n");
			//	recvMsg[MAX_MESSAGE_LEN + 1]="\0";
				//printf("\nWaiting job from server...\n");
				memset(recvMsg, 0, MAX_MESSAGE_LEN + 1);
				//printf("Please enter message (<=%d charctors) to send to server: ", MAX_MESSAGE_LEN);
				delay(1);
			//}

		}

		
	}
		
}





int main(int argc, char *argv[]) {
	/***
	Check command line argument and configure socket address structure and ask for message.
	***/
	struct sockaddr_in addr;
	//struct timeval timeout;
	/**
    struct sockaddr_in{
        short sin_family; //Address family
        unsigned short sin_port; //Port number(¥²¶·­n±Ä¥Îºô¸ô¼Æ¾Ú®æ¦¡,´¶³q¼Æ¦r¥i¥H¥Îhtons()¨ç¼ÆÂà´«¦¨ºô¸ô¼Æ¾Ú®æ¦¡ªº¼Æ¦r)
        struct in_addr sin_addr; //IP address in network byte order¡]Internet address¡^
        char sin_zero[8]; //¬°¤FÅýsockaddr»Psockaddr_in¨â­Ó¼Æ¾Úµ²ºc«O«ù¤j¤p¬Û¦P¦Ó«O¯dªºªÅ¦r¸`
    }; */

    int numbytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	char *port;
	bzero(&addr, sizeof(addr));
	int cbytes=0;

	/*if(argc != 2 || //(addr.sin_addr.s_addr = inet_addr("140.113.1.1"/*strtok(argv[1], ":"))) == INADDR_NONE /*|| (port = strtok(NULL, ":")) == NULL) {
            /** inet_addr() : Change IP w/ dot into 32 bit integer*/
		/*puts("Usage: echo_client xxx.xxx.xxx.xxx:xxxx");
		puts("                   ^ Server IP     ^ Port");
		exit(1);
	} else {
		addr.sin_port = htons(53/*atoi("10000"port));
		addr.sin_family = AF_INET/** TCP/IP ;
	}*/

	if (argc != 2) {
		fprintf(stderr,"Usage: ./client [Domain name of server]\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	char sendMsg[MAX_MESSAGE_LEN + 1]="\0";
	char sendName[MAX_MESSAGE_LEN + 1];
	
	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	    return 1;
  	}
	//printf("Server's Port Number is %d\n",atoi(port));
	//printf("Server's IP is %s\n",strtok(argv[1], ":"));
	

	for(p = servinfo; p != NULL; p = p->ai_next) {

			/***
			Create socket.
			***/
			puts("Creating socket...");
			s_fd = socket(AF_INET, SOCK_STREAM, 0);///int socket(int domain, int type, int protocol),if wrong => return -1

			if(s_fd == -1) {
				puts("Fail to create socket.");
				exit(2);
			} else {
				puts("Socket created");
			}
			
			/***
			Connect.
			***/
			

			printf("Connecting to %s:%s...\n", argv[1], PORT);

			if(connect(s_fd, p->ai_addr, p->ai_addrlen/*(struct sockaddr *) &addr, sizeof(addr)*/) == -1) { ///int connect(int fd, struct sockaddr *remote_host, socklen_t addr_length)
				puts("Fail to connect.");
				exit(2);
			} 

				//send(s_fd, rand_str, strlen(rand_str), 0);
			puts("Server connected.\n\n");
			break;

		}

	freeaddrinfo(servinfo);


	/***
	Creat thread for receiving message to server.
	***/
	pthread_t t1;
	pthread_create(&t1, NULL, &receive, NULL);
	/* Reduce CPU usage */
    sleep(1);

	

	while(1) {	

		


		/***
		Send message to server.
		***/
		//printf("Please enter message (<=%d charctors) to send to server: ", MAX_MESSAGE_LEN);
		scanf("%s", sendMsg);
		//printf("Sending '%s' to server...\n", sendMsg);
		

		if(send(s_fd, sendMsg, strlen(sendMsg), 0) == -1) {
			puts("Fail to send message to server.");
			exit(2);
		} 
		/*else {
			puts("Message sent.");
		}*/
		memset(sendMsg, 0, MAX_MESSAGE_LEN + 1);


		
		/*
		if(recv(s_fd, recvMsg, MAX_MESSAGE_LEN, 0) == -1) {
			puts("Fail to do the job from server.");
			exit(2);
		} else {
			printf("The job from server is :\n\t%s\n", recvMsg);
			recvMsg[cbytes] = '\0';
			printf("\nDoing job sented from server...\n");
			int flag=1;
			while(flag){ 
				printf("Finish yet? 1.yes or 2.no:");
				scanf("%d",&flag);
				
				flag=flag-1;
			} 
		}*/
		
		//delay(1); 
		//timeout.tv_sec = 1500000000;
	}
	return 0;
}


