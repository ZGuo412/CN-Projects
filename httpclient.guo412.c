#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINE 512

int open_clientfd(char *hostname, int port) 
{ 
	int clientfd; 
	struct hostent *hp; 
	struct sockaddr_in serveraddr; 

	if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		return -1; /* check errno for cause of error */ 

	/* Fill in the server's IP address and port */ 
	if ((hp = gethostbyname(hostname)) == NULL) 
		return -2; /* check h_errno for cause of error */ 
	bzero((char *) &serveraddr, sizeof(serveraddr)); 
	serveraddr.sin_family = AF_INET; 
	bcopy((char *)hp->h_addr,  
			(char *)&serveraddr.sin_addr.s_addr, hp->h_length); 
	serveraddr.sin_port = htons(port); 

	/* Establish a connection with the server */ 
	if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		return -1; 
	return clientfd; 
} 

int getLine(char* filename)
{
	FILE * fp;
	fp = fopen("output1.txt", "r");
	char * buf;
	size_t size = 512;
	int count = 0;
	bzero(filename, MAXLINE);
	while((buf = getline(&filename, &size,fp)) != -1){
		if (count == 0){
			if(!(strstr(filename, "200 OK"))){
				exit(0);
			}
		}
		fputs(filename,stdout);
		count++;		
	}
//	printf("!!!%s!!!",filename);
	if (strstr(filename,"\n")){
		char* temp = strstr(filename,"\n");
//		printf("!!!%s!!!",temp);
		strcpy(temp,"\0");
	}
	fclose(fp);
	return count;
}

/* usage: ./echoclient host port */
int main(int argc, char **argv)
{ 
	int clientfd, port; 
	char *host, buf[MAXLINE];
	char temp[MAXLINE];
	char *pathname;
	char *http;
	FILE* fp;
	char filename[MAXLINE];
	char *final_http;
	int n_bytes;
	host = argv[1]; 
	port = atoi(argv[2]);
	pathname = argv[3];
	http = (char *)malloc(MAXLINE);
	final_http = (char *)malloc(MAXLINE);
	strcpy(http, "GET ");
	strcat(http, pathname);
	strcat(http, " HTTP/1.0\r\n\r\n");
	clientfd = open_clientfd(host, port);
	if (clientfd < 0){
		printf("Error opening connection \n");
		exit(0);
	}
	write(clientfd, http, strlen(http));
	fp = fopen("output1.txt", "w");
	//First read the file header and put into an output file
	while(read(clientfd, buf, MAXLINE)){
		fputs(buf, fp);
	//	fputs(buf,stdout);
	}
	fclose(fp);	
	close(clientfd); 
	free(http);
	int n_line = getLine(filename);
	strcpy(final_http, "GET ");
	strcat(final_http,filename);
	strcat(final_http," HTTP/1.0\r\n\r\n");
//	printf("*****%s*****",final_http);
	clientfd = open_clientfd(host, port);
	write(clientfd,final_http, strlen(final_http));
	bzero(temp,MAXLINE);
	while((n_bytes = read(clientfd, temp, MAXLINE - 1))){
		temp[n_bytes] = '\0';
		fputs(temp,stdout);
		bzero(temp, MAXLINE);
	}
	close(clientfd);
	free(final_http);
	exit(0); 
} 

