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
#define NMB 5
//the user struture that contains his informations (pseudo, port, ip address and time of connection)
struct user{
	char pseudo[1024];
	int port;
	char ip_address[1024];
	char time[25];
	char groupes[NMB][1024];
	int o; //number of groupes the user has joined
};

void error(const char *msg)
{
	perror(msg);
	exit(1);
}
//create the socket
int do_socket(){
	int s;
	s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s==-1){
		error("socket");
	}
	printf("Socket Created\n");
	return s;
}

//bind the socket with the server address
void do_bind(int sock,struct sockaddr_in s){
	int b;
	b=bind(sock,(struct sockaddr *)&s,sizeof(s));
	if(b==-1){;
		error("bind");
	}
	printf("bind done\n");
}
//accept the client's connection request and create the socket 
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
//receive message from the client
int do_read(int i, char message[1024]){
	//clean the message because used 
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
//send to the client
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
	char message[1024],who[1024]="/who\n",message2[10],message3[1024],whois[1024]="/whois ",nick[1024]="/nick ",msgall[1024]="/msgall ", msg[1024]="/msg ",create[1024]="/create ",join[1024]="/join ",mer[1024],channel[1024]="/channel ";
	char* message1; 
	char* pos;
	char ser[1024]="";
	char mall[1024]="";
	char ser1[1024]="[server] : ";
	char mall1[1024]="[";
	char mall2[1024]="] : ";
	char un[1024];
	char un1[1024]="";
	int fdmax, i,j,k, sock, sock_client, size,t,v,r,x,y,p,z;
	struct sockaddr_in servaddr, clientaddr;
	fd_set fds;
	fd_set fd_read;
	int max_listen=5;
	time_t ticks; //time structure to contain the user's time of connection
	int nmb_channels=5;
	char grps[NMB][1024];
	
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
		//each time the set of file descriptors ready for reading is reinitialized with the old ones and the added ones
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
					//if the file descriptor is max_listen+4 we know it's the 21st client whos trying to connect because 						//the firstclient to connect gets file descriptor 4 (the 3 first ones are for stdin stdout and stderr)
					//we will never have a file descriptor number>max_listen+4 because the number max_listen+4 is always 						//cleared
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
						
						u[sock_client-4].o=0;
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
						//check if message starts with '/channel'
						if(strstr(message,channel)!=NULL){
							t=0;
							x=0;
							y=0;
							z=0;
							strcpy(un,message+9);
							while(un[t]!=' '){
								un1[t]=un[t];
								t++;
							}
							un1[t]=un[t];
							strcpy(un,un+t+1);
							pos=strchr(un1,'\0');
							*(pos-1)=0;
							//loop over existing channels to determin wether or not the channel mentionned exists
							for(v=0;v<NMB;v++){
								strcpy(mer,grps[v]);
								pos=strchr(mer,'\0');
								*(pos-1)=0;
								if(strcmp(un1,mer)==0)
									x=1;
								memset (mer, 0, sizeof (mer));
							}
							//loop over client's channels to check wether he joined or not the mentioned channel
							for(v=0;v<u[i-4].o;v++){
								strcpy(mer,u[i-4].groupes[v]);
								pos=strchr(mer,'\0');
								*(pos-1)=0;
								if(strcmp(un1,mer)==0)
									y=1;
								memset (mer, 0, sizeof (mer));
							}
							//loop over clients flie descriptors to determin other clients on the same channel
							for(k = 4; k <= fdmax; k++){
								if (FD_ISSET(k, &fds) && k!=i && y==1){
									//loop over each client's channels and send message to those joined
									for(p=0;p<u[k-4].o;p++){
										strcpy(mer,u[k-4].groupes[p]);
										pos=strchr(mer,'\0');
										*(pos-1)=0;
										if(strcmp(un1,mer)==0){
											z=1;
											strcat(mall,mall1);
											strcat(mall,un1);
											strcat(mall,mall2);
											strcat(mall,mall1);
											strcat(mall,u[i-4].pseudo);
											strcat(mall,mall2);
											strcat(mall,un);
											do_send(k, mall, sizeof(mall));
											memset (mer, 0, sizeof (mer));
										}
									}
								}
							}
							//if channel does not exist
							if(x==0){
								strcat(mall,ser1);
								strcat(mall,"channel does not exist");
								do_send(i, mall, sizeof(mall));
							}
							//if the channel exists but the client haven't joined it yet
							if(x==1&&y==0){
								strcat(mall,mall1);
								strcat(mall,un1);
								strcat(mall,mall2);
								strcat(mall,"you haven't joined this channel yet");
								do_send(i, mall, sizeof(mall));
							}
							//if the client is all alone in the channel
							if(x==1&&y==1&&z==0){
								strcat(mall,mall1);
								strcat(mall,un1);
								strcat(mall,mall2);
								strcat(mall,"there are no other users in this channel");
								do_send(i, mall, sizeof(mall));
							}
							memset (un, 0, sizeof (un));
							memset (un1, 0, sizeof (un1));
							memset (mall, 0, sizeof (mall));
						}
						//check if the message starts with '/create'
						else if(strstr(message,create)!=NULL){
							if(v<NMB){
								y=0;
								//loop over existing channels to determin if channel name already exist
								for(x=0;x<v;x++){
									if(strcmp(message+8,grps[x])==0){
										y=1;
										strcat(ser,ser1);
										strcat(ser,"Channel already exists");
										do_send(i,ser,22+11);
									}
								}
								//tell the channel he succeded in creating the channel
								if(y==0){
									strcpy(grps[v],message+8);
									strcat(ser,ser1);
									strcat(ser,"You have created channel: ");
									strcat(ser,message+8);
									do_send(i,ser,size-8+26+11);
									v++;
								}
							
							}
							//if channels number is bigger than server's capacity
							else{
								strcat(ser,ser1);
								strcat(ser,"No more channels can be created");
								do_send(i,ser,31+11);
							}
							memset (ser, 0, sizeof (ser));	
						}
						//check if message starts with '/join'
						else if(strstr(message,join)!=NULL){
							y=0;
							//loop over the channels to check availability
							for(r=0;r<NMB;r++){
								if(strcmp(grps[r],message+6)==0){
									y=1;
									//loop over the clients channels to check if already joined
									for(p=0;p<u[i-4].o;p++){
										if(strcmp(grps[r],u[i-4].groupes[p])==0){
											y=2;
											strcat(ser,ser1);
											strcat(ser,"Channel already joined");
											do_send(i,ser,22+11);
										}
									}
									//tell the client he succeded joining the channel
									if(y==1){
										strcpy(u[i-4].groupes[u[i-4].o],grps[r]);
										u[i-4].o++;
										strcat(mall,mall1);
										strcpy(mer,grps[r]);
										pos=strchr(mer,'\0');
										*(pos-1)=0;
										strcat(mall,mer);
										strcat(mall,mall2);
										strcat(mall,"You have joined channel: ");
										strcat(mall,grps[r]);
										do_send(i,mall,size-6+25+5+strlen(grps[r]));
										memset (mall, 0, sizeof (mall));
									}
								}
							}
							//if channel does not exist
							if(y==0){
								strcat(ser,ser1);
								strcat(ser,"Channel does not exist");
								do_send(i,ser,22+11);
							}
							memset (ser, 0, sizeof (ser));
						}
						//check if message starts with '/msg'
						else if(strstr(message,msg)!=NULL){
							t=0;
							strcpy(un,message+5);
							while(un[t]!=' '){
								un1[t]=un[t];
								t++;
							}
							un1[t]=un[t];
							strcpy(un,un+t+1);
							//since the message gotten from the client contains \n in the end we should delete it 								//in order to compare it
							pos=strchr(un1,'\0');
							*(pos-1)=0;
							//loop over all the file descriptors to send to the right client
							for(k = 4; k <= fdmax; k++){
								if (FD_ISSET(k, &fds) && strcmp(un1,u[k-4].pseudo)==0){
									strcat(mall,mall1);
									strcat(mall,u[i-4].pseudo);
									strcat(mall,mall2);
									strcat(mall,un);
									do_send(k, mall, size-5+strlen(u[i-4].pseudo));
									memset (mall, 0, sizeof (mall));
								}
							}
							memset (un, 0, sizeof (un));
							memset (un1, 0, sizeof (un1));
						}
						//check if message starts w/th '/msgall'
						else if(strstr(message,msgall)!=NULL){
							//loop over all file descriptors to send to all other clients
							for(k = 4; k <= fdmax; k++){
								if (FD_ISSET(k, &fds)){
									if (k != i) {
										strcat(mall,mall1);
										strcat(mall,u[i-4].pseudo);
										strcat(mall,mall2);
										strcat(mall,message+8);
										do_send(k, mall, size-8+5+strlen(u[i-4].pseudo));
										memset (mall, 0, sizeof (mall));
									}
								}
							}
						}
						else{
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
							strcat(ser,ser1);
							strcat(ser,message);
							do_send(i,ser , size+11);
							//clean the string to use it again later
							memset (ser, 0, sizeof (ser));
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
