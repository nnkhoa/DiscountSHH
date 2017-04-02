#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <limits.h>

int main(int argc, char** argv){
	struct sockaddr_in saddr;		//address format of the client
	struct hostent *h;				//host
	int sockfd;				//the socket
	unsigned int port = 6666;		//port to be use
	char hostname[256], buffer[128];
	char *temp_ch;
	int read_value = 0;

	//check for argument
	if(argc > 2){				
		printf("Too many arguments\n");
		printf("Exiting...\n");
		return -1;
	}else if(argc == 2){
		strcpy(hostname, argv[1]);
	}else{
		printf("Enter hostname: ");
		scanf("%s", hostname);
	}

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){		//create new socket
		printf("Error creating socket\n");
		return -1;
	}

	if((h=gethostbyname(hostname)) == NULL){
		printf("Unknown hostname\n");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));		//initialize saddr
	saddr.sin_family = AF_INET;			//addr family
	memcpy((char *)&saddr.sin_addr.s_addr, h->h_addr_list[0], h -> h_length);
	struct in_addr **addr_list = (struct in_addr **)h -> h_addr_list;
	struct in_addr ip = *addr_list[0];
	saddr.sin_port = htons(port);
	printf("%s\n", inet_ntoa(ip));

	if((connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr))) < 0 ){
		printf("Cannot connect\n");
		return -1;
	}

	printf("Connected!\n");

	int ch;

	if(argc != 2){
		while(((ch = getchar()) != EOF) && (ch != '\n'));
	}
		
	while(1){

		memset(buffer, 0, sizeof(buffer));
		
		printf(">Client: ");
		if(fgets(buffer, sizeof(buffer), stdin) == NULL){
			return -1;
		}

		if((strcmp(buffer, "/quit\n") == 0) || (strcmp(buffer, "exit\n") == 0)){
			shutdown(sockfd, SHUT_RDWR);
			close(sockfd);
			printf("Exitting....\n");
			break;
		}

		send(sockfd, buffer, strlen(buffer), 0);

		while(1){
			memset(buffer, 0, sizeof(buffer));
			read_value = read(sockfd, buffer, sizeof(buffer));
			temp_ch = strchr(buffer, '\1');
			if(temp_ch != NULL) {
				*temp_ch = '\0';
				printf("%s", buffer);
				printf("\n");
				break;
			}
			write(1, buffer, read_value);
		}
	}

	return 0;

}