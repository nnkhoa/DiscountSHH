#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

int main(){
	int sockfd, clientlen, clientfd;		//original socket, size of client address, returned client socket
	int const MAX_CLIENT = 100;				//max client that can connect to this server
	int clientfds[MAX_CLIENT];				//the set of clients socket that connect to this server
	struct sockaddr_in saddr, caddr;		//address format of server/client
	unsigned port = 6666;					//port to be used
	char buffer[256], temp[256];			//buffer to use to send/receive message

	memset(clientfds, 0, sizeof(clientfds));	//initialize the array of clientfd

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){			//create new socket
		printf("Error creating socket!\n");
		return -1;
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));		//reuse socket

	//make server non-blocking
	int fl = fcntl(sockfd, F_GETFL, 0);		
	fl |= O_NONBLOCK;
	fl = fcntl(sockfd, F_SETFL, fl);

	memset(&saddr, 0, sizeof(saddr));		//initialize saddr
	saddr.sin_family = AF_INET;			//address family
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);		//internet address in network byte order
	saddr.sin_port = htons(port);	//port in network byte order

	if((bind(sockfd, (struct sockaddr *) &saddr, sizeof(saddr))) < 0 ){		//bind socket
		printf("Error in binding\n");
		close(sockfd);
		return -1;
	}

	if(listen(sockfd, 5) < 0){			//listen on socket
		printf("Error in listening\n");
		close(sockfd);
		return -1;
	}

	printf("Waiting for connection...\n");

	clientlen = sizeof(caddr);    //get sizeof client address

	while(1){
		fd_set set;			//file descriptor set
		FD_ZERO(&set);		//clear the set
		FD_SET(sockfd, &set);		//add listening sockfd to the set

		int maxfd = sockfd;

		for(int i = 0; i < MAX_CLIENT; i++){
			if(clientfds[i] > 0){			//if is a valid fd, add it to the set
				FD_SET(clientfds[i], &set);
			}

			if(maxfd < clientfds[i]){		//find maxfd
				maxfd = clientfds[i];
			}
		}

		select(maxfd + 1, &set, NULL, NULL, NULL);		//poll and wait, block indefinitely

		if(FD_ISSET(sockfd, &set)){
			
			clientfd = accept(sockfd, (struct sockaddr *) &caddr, &clientlen);

			//make client non-blocking
			int cfl = fcntl(clientfd, F_GETFL, 0);
			cfl |= O_NONBLOCK;
			cfl = fcntl(clientfd, F_SETFL, cfl);

			for(int i = 0; i < MAX_CLIENT; i++){
				if(clientfds[i] == 0){
					clientfds[i] = clientfd;
					break;
				}
			}	
		}

		for(int i = 0; i < MAX_CLIENT; i++){
			if(clientfds[i] > 0 && FD_ISSET(clientfds[i], &set)){
				memset(buffer, 0, sizeof(buffer));
				if(read(clientfds[i], buffer, sizeof(buffer)) > 0){
					printf("Client %d said: %s\n", i, buffer);
				} else{
					printf("Client %d has disconnected\n", i);
					clientfds[i] = 0;
				}
			}
		}

	}

}