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
	return s;
}

void do_connect(int s, struct sockaddr_in sock){
	int c;
  	c=connect(s,(struct sockaddr *)& sock,sizeof(sock));
	if(c==-1){
		perror("connection");
		exit(EXIT_FAILURE);
	}
}

void readline(char message[100]){
	fgets(message, 100, stdin);
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
	r=recv(sock ,rep , 100 , 0);
	if(r==-1){
		perror("recv");
		exit(EXIT_FAILURE);
	}
	rep[r]='\0';
	printf("[server] : %s\n",rep);
}

int main(int argc,char** argv){

	int sock, fdmax, i;
	struct sockaddr_in servaddr;
	char message[100];
	char reply[100];
	fd_set fds;
	fd_set fd_read;
	
	if (argc != 3){
		fprintf(stderr,"usage: RE216_CLIENT hostname port\n");
		return 1;
	}
	
	//get address info from the server
	servaddr=get_addr_info(argv[1], argv[2]);

	//create the socket
	sock =do_socket();
	
	//connect to remote socket
	do_connect(sock,servaddr);
	
	//clear the file descriptor that will store the socket for user input and the socket for server reception
	FD_ZERO(&fds);
	//clean the file descriptor where we will store file descriptors ready for reading
        FD_ZERO(&fd_read);
	//add the user-input socket 
        FD_SET(0, &fds);
	//add the server reception socket
        FD_SET(sock, &fds);
	//the highest file descriptor number corresponds to the one for server reception
	fdmax = sock;
	
	//receve the servers comment on whether the client goes beyond the server's maximum of incoming connections or not
	recv(sock ,reply , 100 , 0);
	//if the client is beyond the limit ask him to try again when another already connected client quit and exit the program
	if(strcmp(reply,"beyond limit")==0){
		printf("Server cannot accept incoming connections anymore. Try again later.\n");
		exit(0);
	}
	
	//the first "enter your message" request
	printf("enter your message:\n");

	while(1){
		//each time the set of file descriptors ready for reading is reinitialized
		fd_read = fds;
		//wait for any file descriptor to be available for reading
		if(select(fdmax+1, &fd_read, NULL, NULL, NULL) == -1){
			perror("select");
			exit(EXIT_FAILURE);
		}
		//loop over available file descriptors 
		for(i=0; i <= fdmax; i++ ){
			// check if the file descriptor is open for reading
			if(FD_ISSET(i, &fd_read)){
				//if it's a user input get the message and send it to the server or exit the program on user's request
				if (i == 0){
					readline(message);
					if (strcmp(message , "/quit\n") == 0) {
						printf("[server] : You will be terminated\n"); 
						printf("Connection terminated\n");
						exit(0);
					}else
						handle_client_message(sock, message);
				//else it's a server input so read from the server and write on user output
				}else {
					do_recv(sock, reply);
					//ask for new message
					printf("enter your message:\n");

				}
			}
		}
	}
	//clean up socket
	close(sock);
	return 0;
}
