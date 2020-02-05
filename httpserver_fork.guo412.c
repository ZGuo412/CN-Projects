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
#define LISTENQ 10
#define MAXLINE 512

int open_listenfd(int port)  
{ 
	int listenfd, optval=1; 
	struct sockaddr_in serveraddr; 

	/* Create a socket descriptor */ 
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		return -1; 

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
	if (listen(listenfd, LISTENQ) < 0) 
		return -1; 

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
//	while((n = read(connfd, buf, MAXLINE)) != 0) { 
//		printf("server received %d bytes\n", n); 
//		write(connfd, buf, n); 
//	} 
} 


int main(int argc, char **argv) {
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp;
	int childpid;
	port = atoi(argv[1]); /* the server listens on a port passed 
							 on the command line */
	listenfd = open_listenfd(port); 

	while (1) {
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
