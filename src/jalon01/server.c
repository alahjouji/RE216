#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg)
{
	perror(msg);
	exit(1);
}
  
int do_socket(){
	int s;
	s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s==-1){
		error("socket");
	}
	printf("Socket Created\n");
	return s;
}

void init_serv_addr(struct sockaddr_in s,char* port){
	s.sin_family=AF_INET;
	s.sin_port = htons(atoi(port));
	s.sin_addr.s_addr=INADDR_ANY;
}

void do_bind(int sock,struct sockaddr_in s){
	int b;
	b=bind(sock,(struct sockaddr *)&s,sizeof(s));
	if(b==-1){;
		error("bind");
	}
	printf("bind done\n");
}

int do_accept(int s,struct sockaddr_in sock){
	int a;
	int b=sizeof(struct sockaddr_in);
	a = accept(s,(struct sockaddr *) &sock,(socklen_t*)&b);
	if(a==-1){
		error("accept");
	}
	printf("connection accepted\n");
	return a;
}

void do_read(int s, char message[100]){
	int r;
	bzero(message,100);
	r=recv(s,message,100,0);
	if(r==-1){
		error("reception");
	}
	printf("message recieved\n");
}

void do_write(int s, char message[100]){
	ssize_t w;
	write(s, message, strlen(message));
	if(w< 0){
		error("write");
	}
	printf("message written\n");
}

int main(int argc, char** argv){

	struct sockaddr_in servaddr,clientaddr;
	int sock,sock_client,c;
	int b=sizeof(struct sockaddr_in);
	char message[100],set[100] = "",quit[]="/quit";

	if (argc != 2){
		fprintf(stderr, "usage: RE216_SERVER port\n");
		return 1;
	}


	//create the socket, check for validity!
	//do_socket()
	sock=do_socket();

	//init the serv_add structure
	//init_serv_addr()
	//init_serv_addr(servaddr,argv[1]);
	memset (&servaddr,'\0', sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port = htons(atoi(argv[1]));
	servaddr.sin_addr.s_addr=INADDR_ANY;

	//we bind on the tcp port specified
	//do_bind()
	do_bind(sock,servaddr);

	//specify the socket to be a server socket and listen for at most 20 concurrent client
	//listen()
	listen(sock,20);

	for (;;){

		//accept connection from client
		//do_accept()
		sock_client=do_accept(sock,clientaddr);

		//repeat the process while the client doesn't try to close the connection
		while(strcmp(message,quit)!=0){

			//read what the client has to say
			//do_read()
			do_read(sock_client,message);

			//break because client wrote "/quit"
			if(strcmp(message,quit)==0){
					break;
			}

			//we write back to the client
			//do_write()
			do_write(sock_client, message);
		}
		
		printf("client disconnected\n");

		//reset the message for next connections
		strcpy(message, "");

		//clean up client socket
		close(sock_client);
		

	}

	//clean up server socket
	close(sock);

	return 0;
}
