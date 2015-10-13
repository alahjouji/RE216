#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
	
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

int do_read(int i, char message[100]){
	int r;
	r= recv(i, message, 100, 0);
	printf("message recieved from socket %d\n", i);
	if (r<= 0) {
		if (r == 0) {
			printf("socket %d quited\n", i);
		}else{
			error("reception");
		}
	}
		
	return r;
}

void do_send(int i, char message[100], int size){
	int s;
	s=send(i, message, size, 0);
	if(s< 0){
		error("send");
	}
	printf("message sent to socket %d\n", i);
}

int main(int argc, char** argv){		
	char message[100];
	int fdmax, i, sock, sock_client, size;
	struct sockaddr_in servaddr, clientaddr;
	fd_set fds;
	fd_set fd_read;
	int max_listen=20;

	if (argc != 2){
		fprintf(stderr, "usage: RE216_SERVER port\n");
		return 1;
	}
	
	////create the socket
	sock = do_socket();
		
	//init the serv_add structure
	memset (&servaddr,'\0', sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port = htons(atoi(argv[1]));
	servaddr.sin_addr.s_addr=INADDR_ANY;
		
	//bind on the tcp port specified
	do_bind(sock,servaddr);
	
	//specify the socket to be a server socket and listen for at most 20 concurrent client
	if (listen(sock, max_listen) == -1) {
		perror("listen");
		exit(1);
	}
	//clear the file descriptor that will store the socket for server reception
	FD_ZERO(&fds);
	//clean the file descriptor where we will store file descriptors ready for reading
	FD_ZERO(&fd_read);
	//add the server reception socket
	FD_SET(sock, &fds);
	//the highest file descriptor number corresponds to the one for server reception
	fdmax = sock;

	for (;;){
		//each time the set of file descriptors ready for reading is reinitialized
		fd_read = fds;
		//wait for any file descriptor to be available for reading
		if(select(fdmax+1, &fd_read, NULL, NULL, NULL) == -1){
			perror("select");
			exit(EXIT_FAILURE);
		}
		
		//loop over available file descriptors 
		for (i = 0; i <= fdmax; i++){
			// check if the file descriptor is open for reading
			if (FD_ISSET(i, &fd_read)){
				//the server is listening on this socket which means it's a new connection
				if (i == sock){
					//accept connection from client
					sock_client = do_accept(sock,clientaddr);
					//check the file descriptor number to know the client's number
					//if the file descriptor is 24 we know it's the 21st client whos trying to connect because the first
					//client to connect gets file descriptor 4 (the 3 first ones are for stdin stdout and stderr)
					//we will never have a file descriptor number>24 because the number 24 is always cleared
					if(sock_client==max_listen+4){
						//alarm the client that he's the 21st one
						do_send(sock_client, "beyond limit", 12);
						//close the client socket to clear file descriptor number 24
						close(sock_client);
						//get out of the loop because highest number possible reached
						break;
						}
					else{
						//tell the client that he is not over the maximum client limit
						do_send(sock_client, "not over limit", 14);
						//add the client socket to the file descriptors opened for reading 
						FD_SET(sock_client, &fds);
						//change the highest file descriptor number if necessary
						if(sock_client > fdmax){
							fdmax = sock_client;
						}
					}
				}else{
					//read what the client has to say
					size=do_read(i,message);
					//if size<=0 means either error reading or client typed "/quit"
					if (size <= 0) {
						//close the socket corresponding to the client who quitted
						close(i);
						//clear the file descriptor from the set of file descriptors open for reading
						FD_CLR(i, &fds);
					}else{ 	
						if (FD_ISSET(i, &fds)){
							//we write back to the client
							do_send(i, message, size);
						}
					}
				}
			}	
		}
	}
	//clean up server socket
	close(sock);
	return 0;
}
