#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/select.h>
#define LISTENQ 10
#define MAXLINE 512

int open_listenfd(int port,int check)  
{ 
	int listenfd, optval=1; 
	struct sockaddr_in serveraddr; 

	/* Create a socket descriptor */ 
	if (check == 0){
		if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
			return -1;
	}
	//UDP transport(sock_dgram)
	else{    
		if((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			exit(-1);
		}
	}

	/* Eliminates "Address already in use" error from bind. */ 
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,  
				(const void *)&optval , sizeof(int)) < 0) 
		return -1; 

	/* Listenfd will be an endpoint for all requests to port 
	 *      on any IP address for this host */ 
	bzero((char *) &serveraddr, sizeof(serveraddr)); 
	serveraddr.sin_family = AF_INET;  
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);  
	serveraddr.sin_port = htons((unsigned short)port);  
	if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) 
		return -1; 

	/* Make it a listening socket ready to accept 
	 *      connection requests */ 
	if (check == 0){
		if (listen(listenfd, LISTENQ) < 0) 
			return -1;
	}
//	else{
//		if (listen(
//	}

	return listenfd; 
} 

void echo(int connfd)  
{ 
// errno
	size_t n;  
	char buf[MAXLINE];
	char input[1];
	FILE* fp;
	char httpok[] = "HTTP/1.0 200 OK\r\n\r\n";
	char http404[] = "HTTP/1.0 404 Not Found\r\n\r\n";
	char http403[] = "HTTP/1.0 403 Forbidden\r\n\r\n";
	read(connfd, buf, MAXLINE);
	char* path = strtok(buf, " ");
	path = strtok(NULL, " ");
	int number  = atoi(strtok(NULL," "));
	if (access(path, R_OK) == 0){
		write(connfd, httpok, strlen(httpok));
		fp = fopen(path, "r");
		while((input[0] = fgetc(fp)) != EOF){
			if (isupper(input[0])){
				input[0] = (char)(((int)input[0] - number + 26 - 65)%26 + 65);
				write(connfd, input, 1);
			}
			else if(islower(input[0])){
				input[0] = (char)(((int)input[0] - number + 26 - 97)%26 + 97);
				write(connfd, input, 1);
			}
			else{
				write(connfd, input, 1);
			}
		}
	}
	else{
		if(access(path, F_OK) == 0){
			write(connfd, http403, strlen(http403));
		}
		else{
			write(connfd, http404, strlen(http404));
		}
	} 
} 


int main(int argc, char **argv) {
	int listenfd, connfd, port, clientlen, ping, udp_listenfd;
	struct sockaddr_in clientaddr;
	struct sockaddr_in udpaddr;
	struct hostent *hp;
	struct hostent *udphost;
	char *haddrp;
	int childpid;

	//initianize variables for udp
	fd_set rfds;
	fd_set master;
	struct sockaddr_in serveraddr;
	int fdmax;
	int addrlen;
	int retval;
	uint32_t  seq_num;
	//
	char data[MAXLINE];
	char buf[MAXLINE];
	struct in_addr ipv4addr;
	port = atoi(argv[1]); /* the server listens on a port passed 
							 on the command line */
	ping = atoi(argv[2]);
	listenfd = open_listenfd(port,0); 
	udp_listenfd = open_listenfd(ping,1);
	while (1) {


		//reference:tenouk.com/Module41.html; select()from examples
		FD_ZERO(&rfds);
		FD_ZERO(&master); 
		//rfds will be copied from master, so it doesn't matter
		FD_SET(listenfd, &rfds);
		FD_SET(udp_listenfd, &rfds);
		//keep track the biggest file descriptor//
		if (listenfd > udp_listenfd){fdmax = listenfd;}
		else{fdmax = udp_listenfd;}
		retval = select(fdmax + 1, &rfds, NULL, NULL, NULL);
		if(retval == -1){exit(1);}
		//for UDP, looking for data to be read//



		if(FD_ISSET(udp_listenfd, &rfds)){
			addrlen = sizeof(clientaddr);
			int num_byte = recvfrom(udp_listenfd, buf, MAXLINE, 0,(struct sockaddr *)&udpaddr,&addrlen);
			memcpy(data, buf,num_byte - 4);
			data[num_byte - 4] = '\0';
			memcpy(&seq_num,buf + num_byte - 4, 4);
			seq_num = ntohl(seq_num);
			seq_num += 1;
			//print
			seq_num = htonl(seq_num);
			inet_pton(AF_INET, data, &ipv4addr);
			udphost = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
			if(udphost == NULL){exit(-1);}
			bzero(buf,MAXLINE);
			memcpy(buf, udphost->h_name, strlen(udphost->h_name));
			memcpy(buf + strlen(udphost->h_name), &seq_num, 4); 
			sendto(udp_listenfd,buf, strlen(udphost->h_name) + 4,0,(struct sockaddr *)&udpaddr,addrlen);
			//sendto
	    }
		//////////
		else if(FD_ISSET(listenfd, &rfds)){
			clientlen = sizeof(clientaddr); 
			connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
			hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
					sizeof(clientaddr.sin_addr.s_addr), AF_INET);
			haddrp = inet_ntoa(clientaddr.sin_addr);
			if ((childpid = fork()) == 0){
				close(listenfd);
				echo(connfd);
				exit(0);
			}
			close(connfd);
		}
	}
}
