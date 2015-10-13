#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
//get address info of the server
struct sockaddr_in get_addr_info(char* port, char* adr){
	struct sockaddr_in s;
	memset (&s,'\0', sizeof(s));
	s.sin_family=AF_INET;
	s.sin_port=htons(atoi(port));
	inet_aton(adr,& s.sin_addr);
	return s;
}
//create the socket
int do_socket(){
	int s;
	s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s==-1){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	return s;
}
//connect the socket to the server having the address structure sock
void do_connect(int s, struct sockaddr_in sock){
	int c;
  	c=connect(s,(struct sockaddr *)& sock,sizeof(sock));
	if(c==-1){
		perror("connection");
		exit(EXIT_FAILURE);
	}
}
//get the message from the client
void readline(char message[1024]){
	fgets(message, 1024, stdin);
}
//send the message to the server
void handle_client_message(int sock, char message[1024]){
	int s;
	s=send(sock , message , strlen(message) , 0);
	if(s==-1){
		perror("send");
		exit(EXIT_FAILURE);
	}
}
//receive the message from the server and print it
void do_recv(int sock, char rep[1024]){
	int r;
	r=recv(sock ,rep , 1024 , 0);
	if(r==-1){
		perror("recv");
		exit(EXIT_FAILURE);
	}
	rep[r]='\0';
	printf("%s\n",rep);
}

int main(int argc,char** argv){

	int sock, fdmax, i;
	struct sockaddr_in servaddr;
	char message[1024];
	char reply[1024];
	char pseudo[1024];
	char nick[1024]="/nick ";
	char* pos;	
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
	recv(sock ,reply , 1024 , 0);
	//if the client is beyond the limit ask him to try again when another already connected client quit and exit the program
	if(strcmp(reply,"beyond limit")==0){
		printf("Server cannot accept incoming connections anymore. Try again later.\n");
		exit(0);
	}
	
	//if not the server asks the client to identify himself with a pseudo
	printf("[server] : please logon with /nick <your pseudo>\n");
	//if the client does not follow the instructions below ask him to try again
	do{ 	
		fgets(pseudo, 1024, stdin);
		//if what the user wrote does not contain "/nick " or not
		if(strstr(pseudo,nick)==NULL) 
			printf("[server] : wrong spelling, please logon with /nick <your pseudo>\n");
	}while(strstr(pseudo,nick)==NULL);
	
	//if the syntaxe is correct the pseudo is located 6 caracters after the begining of what the user wrote
	strcpy(pseudo,pseudo+6);
	//since fgets adds '\n' automaticaly to make a new line we need to take it off to get our pseudo  		
	//so we go to the last caracter of the string
	pos=strchr(pseudo,'\0');
	//take off the next to last caracter
	*(pos-1)=0;
	//welcome the client on the chat
	printf("[server] : welcome on the chat %s\n\n",pseudo);
	//send the server the client's pseudo
	handle_client_message(sock , pseudo);
	

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
					//if the client writes '/quit' exit the program
					if (strcmp(message , "/quit\n") == 0) {
						printf("[server] : You will be terminated\n"); 
						printf("Connection terminated\n");
						exit(0);
					}else
						//send what the client wrote to the server
						handle_client_message(sock, message);
				//else it's a server input so read from the server and write on user output
				}else {
					do_recv(sock, reply);
					//if client does not receve from the server like when he sends a message to all other clients  but him 						//or when he sends a message to a certain client or when he sends a message in a channel 
					// he does not receve anything so we empty the tampon and start again
					fflush(stdout);

				}
			}
		}
	}
	//clean up socket
	close(sock);
	return 0;
}
