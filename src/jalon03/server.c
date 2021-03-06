#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h> 

//the user struture that contains his informations (pseudo, port, ip address and time of connection)
struct user{
	char pseudo[1024];
	int port;
	char ip_address[1024];
	char time[25];
};

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

int do_accept(int s,struct sockaddr_in *sock){
	int a;
	int b=sizeof(struct sockaddr_in);
	a = accept(s,(struct sockaddr *) sock,(socklen_t*)&b);
	if(a==-1){
		error("accept");
	}
	printf("connection accepted from %s on port %d\n",inet_ntoa(sock->sin_addr),ntohs(sock->sin_port));
	return a;
}

int do_read(int i, char message[1024]){
	memset(message,0,1024);
	int r;
	r= recv(i, message, 1024, 0);
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

void do_send(int i, char message[1024], int size){
	int s;
	s=send(i, message, size, 0);
	if(s< 0){
		error("send");
	}
	printf("message sent to socket %d\n", i);
}

int main(int argc, char** argv){
	struct user u[20];		
	char message[1024],who[1024]="/who\n",message2[10], message3[1024], whois[1024]="/whois ", nick[1024]="/nick ";
	char* message1; 
	char* pos;
	int fdmax, i,j, sock, sock_client, size;
	struct sockaddr_in servaddr, clientaddr;
	fd_set fds;
	fd_set fd_read;
	int max_listen=20;
	time_t ticks; //time structure to contain the user's time of connection

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
					sock_client = do_accept(sock,&clientaddr);

					//get the actual time which corresponds to the time of the user's connection
					ticks = time(NULL);

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

						//get the client's pseudo and add it to the table of user structure
						//since the client sockets start at 4 (after stdin stdout and stderr and the server's socket) 
						//each table case i containes the informations for the client connected to the socket i+4
						do_read(sock_client,u[sock_client-4].pseudo);
						//add the port on which the client is connected to the table
						u[sock_client-4].port=ntohs(clientaddr.sin_port);
						//add the ip address of the client to the table
						strcpy(u[sock_client-4].ip_address,inet_ntoa(clientaddr.sin_addr));
						//add the time previously gotten which corresponds to the client's connection time to the table
						snprintf(u[sock_client-4].time, sizeof(u[sock_client-4].time), "%.24s\n\r", ctime(&ticks));
					}
				//client previously connected typing a message
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

							//if the message gotten from the client contains '/nick' 
							//he is trying to change his pseudo
							if(strstr(message,nick)!=NULL){
								//the new pseudo starts at the 6th caracter after the begining of the message
								strcpy(message,message+6);
								pos=strchr(message,'\0');
								*(pos-1)=0;
								printf("%s changed his pseudo to %s\n",u[i-4].pseudo,message);
								//change the client's pseudo in his user's structure
								strcpy(u[i-4].pseudo,message);
								//writing the message that will be sent to the client
								strcpy(message,"you changed your pseudo to ");
								message1=strcat(message,u[i-4].pseudo);
								message1=strcat(message1,"\n");
								strcpy(message,message1);
								size=strlen(message);
							}

							//if the message gotten from the client is '/who' 
							//he is trying to get the pseudos of all the connected users
							if(strcmp(message,who)==0){
								strcpy(message,"Online users are: \n");
								//looping over all the available file descriptors
								//since the first 4 file descriptors are taken
								//for each j the corresponding available file descriptor is j+4 
								for(j=0;j<fdmax-3;j++){
									//if the file descriptor is open for writing  										//then the corresponding user is connected so he must be aadded 
									if (FD_ISSET(j+4, &fds)){
										message1=strcat(message,"-");
										//u[j] because the table of users starts at indice 0
										message1=strcat(message,u[j].pseudo);
										message1=strcat(message,"\n");
									}
								}
								strcpy(message,message1);
								size=strlen(message);
									
							}

							//if the message gotten from the client contains '/whois ' 
							//he is trying to get informations about a user
							if(strstr(message,whois)!=NULL){
								//the pseudo of the user the client want informations about starts at the 7th 									//caracter after the begining of the message
								strcpy(message,message+7);
								pos=strchr(message,'\0');
								*(pos-1)=0;
								strcpy(message3,message);
								//looping over all the available file descriptors
								//since the first 4 file descriptors are taken
								//for each j the corresponding available file descriptor is j+4 
								for(j=0;j<fdmax-3;j++){
									//if the file descriptor is open for writing and the pseudo gotten 										//corresponds to a user then show informations about him
									if (FD_ISSET(j+4, &fds) && strcmp(message,u[j].pseudo)==0){
										message1=strcat(message," connected with IP address: ");
										message1=strcat(message1,u[j].ip_address);
										message1=strcat(message1," since ");
										message1=strcat(message1,u[j].time);
										message1=strcat(message1," with port number: ");
										//changing the port from an integer to a string
										sprintf(message2,"%d",u[j].port);
										message1=strcat(message1,message2);
										message1=strcat(message1,"\n");
									}
								}
								//check if the message changed or not, if not then the loop didn't proceed
								//so the user doesn't exist between connected users
								if(strcmp(message,message3)==0){
									strcpy(message,"User searched does not exist");
									size=strlen(message);
								}else{
									strcpy(message,message1);
									size=strlen(message);
								}
							}//we write back to the client
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
