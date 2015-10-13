#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

struct sockaddr_in get_addr_info(char* port, char* adr){
	struct sockaddr_in s;
	memset (&s,'\0', sizeof(s));
	s.sin_family=AF_INET;
	s.sin_port=htons(atoi(port));
	inet_aton(adr,& s.sin_addr);
	return s;
}

int do_socket(){
	int s;
	s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s==-1){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	printf("Socket Created\n");
	return s;
}

void do_connect(int s, struct sockaddr_in sock){
	int c;
  	c=connect(s,(struct sockaddr *)& sock,sizeof(sock));
	if(c==-1){
		perror("connection");
		exit(EXIT_FAILURE);
	}
	printf("Connection established\n");
}

void readline(char message[100]){
	printf("enter your message:\n");
	gets(message);
}
void handle_client_message(int sock, char message[100]){
	int s;
	s=send(sock , message , strlen(message) , 0);
	if(s==-1){
		perror("send");
		exit(EXIT_FAILURE);
	}
}
void do_recv(int sock, char rep[100]){
	int r;
	//avoid '/0' problem
	bzero(rep,100);
	r=recv(sock ,rep , 100 , 0);
	if(r==-1){
		perror("recv");
		exit(EXIT_FAILURE);
		}
	printf("server reply: %s\n",rep);
}

int main(int argc,char** argv){

	int sock;
	struct sockaddr_in servaddr;
	char message[100],reply[100],quit[]="/quit";


	if (argc != 3){
		fprintf(stderr,"usage: RE216_CLIENT hostname port\n");
		return 1;
	}


	//get address info from the server
	//get_addr_info()
	servaddr=get_addr_info(argv[1],argv[2]);

	//get the sockets
	//s = do_socket()
	sock=do_socket();

	//connect to remote socket
	//do_connect()
	do_connect(sock,servaddr);
	
	//repeat the process while the client doesn't write "/quit"
	while(strcmp(message,quit)!=0){

		//get user input
		//readline()
		readline(message);
		
		//send message to the server
		//handle_client_message()
		handle_client_message(sock, message);

		//break because can't recieve if connection closed
		if(strcmp(message,quit)==0){
			printf("server reply: You will be terminated\n"); 
			break;
		}

		//recieve message from server & print it in output
		//do_recv()
		do_recv(sock,reply);
	}

	//clean up socket
	close(sock);
	printf("Connection terminated\n");

	return 0;


}
