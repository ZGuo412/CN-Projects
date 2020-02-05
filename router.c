#include "ne.h"
#include "router.h"
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
pthread_mutex_t lock;
//pthread_mutex_t lock1;
int converge = 0;
FILE* fptr;
int routerID;
char* neHost;
int nePort;
int routerPort;
int UDPfd, sendTo, recvFrom, optval = 1;
struct sockaddr_in neaddr;
struct hostent *hp;
struct sockaddr_in serveraddr;
//al time_val;
//timeval checkcon;
//timeval checkupdate;
//timeval checkdead[MAX_ROUTERS];
//time_t   time(NULL) reall time
struct pkt_INIT_RESPONSE initRecv;
struct pkt_RT_UPDATE RT_request;
struct pkt_RT_UPDATE rec_request;
//int UDPfd;
time_t realtime[MAX_ROUTERS] = {0};
time_t checkcon = 0;
time_t checkupdate = 0;
time_t total = 0;
int detect_convergence_flag = 0;
int detect_nbr_dead[MAX_ROUTERS] = {0};
void * udpfd_polling(void *);
void * update_timer(void *);
int index_r = 0;

void * update_timer(void *arg){
	checkupdate = time(NULL);
	while(1){
		pthread_mutex_lock(&lock);
		time_t now;
		now = time(NULL);
		if(now - checkupdate > UPDATE_INTERVAL){
			bzero((char *)&RT_request, sizeof(RT_request));
			ConvertTabletoPkt(&RT_request, routerID);
			int i;
			for(i = 0; i < initRecv.no_nbr; i++){
				RT_request.dest_id = initRecv.nbrcost[i].nbr;
				hton_pkt_RT_UPDATE(&RT_request);
				sendto(UDPfd, (struct pkt_RT_UPDATE *)&RT_request, sizeof(RT_request),0,(struct sockaddr *)&neaddr, sizeof(neaddr));
				ntoh_pkt_RT_UPDATE(&RT_request);
			}
			checkupdate = time(NULL);
		}
		int b;
		for(b = 0; b < initRecv.no_nbr; b++){
			now = time(NULL);
			if(now - realtime[b] > FAILURE_DETECTION &&detect_nbr_dead[b]==0){
				//reset timer check converage
				checkcon = time(NULL);
				converge=0;
			       	detect_nbr_dead[b] = 1;
				UninstallRoutesOnNbrDeath(initRecv.nbrcost[b].nbr);
				//detect_nbr_dead = 1;//////Morris

				PrintRoutes(fptr,routerID);
			}
		}
		now = time(NULL);
		if(now - checkcon > CONVERGE_TIMEOUT && converge == 0){
			int re = now - total;
//			pthread_mutex_lock(&lock1);
			converge = 1;
			fprintf(fptr,"%d: CONVERGED\n",	re);
			fflush(fptr);
		}
		pthread_mutex_unlock(&lock);
		//update intime or not
		//loop to check nbr dead or not, time - time[index]   if nbr dead update checkcon;
		//checkcon and time > timeout print
	}
}
void * udpfd_polling(void *arg){	
//	pthread_mutex_lock(&lock);
	//for(i=0;i<MAX_ROUTERS;i++){nodeTime[i]=0;}
	while(1){
		recvfrom(UDPfd, (struct pkt_RT_UPDATE *)&rec_request, sizeof(rec_request),0,NULL,NULL);
		pthread_mutex_lock(&lock);
//		time = time_val.tv_sec;
		
		ntoh_pkt_RT_UPDATE(&rec_request);
		int i;
		for(i=0;i<initRecv.no_nbr;i++){
			if(rec_request.sender_id == initRecv.nbrcost[i].nbr){
				index_r = i;
			}
		}
		realtime[index_r] = time(NULL);
		//update the time;
//		checkdead[index_r] = time;
		//check if needs update
		detect_nbr_dead[index_r]=0;
		if(UpdateRoutes(&rec_request, initRecv.nbrcost[index_r].cost ,routerID) && detect_nbr_dead[index_r] == 0){//MORRIS
				PrintRoutes(fptr,routerID);
				//reset converage timer;
//				pthread_mutex_unlock(&lock1);
				converge = 0;
				checkcon = time(NULL);
			//	fflush(fptr);		
		}
		
/*		else{//MORRIS
		  //converge = 1;//or should I use dead neighbor
		 // detect_nbr_dead[MAX_ROUTERS] = {1};
		  return NULL;
		}*/

		pthread_mutex_unlock(&lock);
	}
//	pthread_mutex_unlock(&lock);
	return NULL;
}



//• router <router id> <ne hostname> <ne UDP port> <router UDP port>
//• ne <ne UDP Port> <ConfigFile>

//When a router (to be implemented in file router.c) starts up, it sends an INIT_REQUEST message to the Network Emulator,
// which includes only its router-id. The router’s ID should be between 0 and MAX_ROUTERS – 1 , where MAX_ROUTERS is the 
// maximum number of routers that the system can support. This ID is fed by you in the command prompt as an argument when
// you run your router binary so make sure you supply consistent values.

//The Network Emulator waits until receiving an INIT_REQUEST from ALL routers (to ensure all routers are alive before exchanging messages),
// after which it sends each router an INIT_RESPONSE message that includes information regarding the neighbors of the router, and the cost 
// of the links to the neighbors. The Network Emulator stores a mapping between the router id and router port so it can properly forward 
// any packet tagged with a destination router id.

//main
//sends an INIT_REQUEST message to the ne
//Waiting to receive pkt_INIT_RESPONSE
//send RT_UPDATE every UPDATE_INTERVAL(1) second, after UDP receive RT_UPDATE -> calls UpdateRoutes at the same time track receiving interval for last update
//Use thread to check UDPfd
//TIME: We consider a routing table to have converged if it has not been modified for CONVERGE_TIMEOUT seconds, even though several update messages may have been received.
//UPDATE_INTERVAL 1 /* router sends routing updates every 1 sec */
//FAILURE_DETECTION (UPDATE_INTERVAL * 3) /* not receiving a nbr's update more than 3 update cycles = 3 * UPDATE_INTERVAL, consider it dead
//CONVERGE_TIMEOUT (UPDATE_INTERVAL * 5) /* if routing table is not changed after 5 update cycles, assume converged */
//int routerID;
//char* neHost;
//nt nePort;
//int routerPort;
//int UDPfd, sendTo, recvFrom, optval = 1;
//struct sockaddr_in neaddr;
//struct hostent *hp;
//struct sockaddr_in serveraddr;
int main(int argc, char ** argv){
	if(argc != 5){return -1;}
	routerID = atoi(argv[1]);
	neHost = argv[2];
	nePort = atoi(argv[3]);
	routerPort = atoi(argv[4]);
//	int UDPfd, sendTo, recvFrom, optval = 1;
//	struct sockaddr_in neaddr;
//	struct sockaddr_in serveraddr;
//	struct hostent *hp;

	if(routerID < 0 || routerID > (MAX_ROUTERS - 1)){return -1;}
	//UDP
	if((UDPfd = socket(AF_INET, SOCK_DGRAM, 0))<0){return -1;}
	if((hp = gethostbyname(neHost))==NULL){return -2;}
	if(setsockopt(UDPfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval, sizeof(int))<0){return -1;}
	
	//build ne address
	bzero((char *) &neaddr, sizeof(neaddr));

	neaddr.sin_family = AF_INET;
	bcopy((char *)hp->h_addr,(char*)&neaddr.sin_addr.s_addr,hp->h_length);
	neaddr.sin_port = htons((unsigned short)nePort);
	bzero((char *) &serveraddr, sizeof(serveraddr));

	
	serveraddr.sin_family = AF_INET; 
   	 serveraddr.sin_addr.s_addr = INADDR_ANY; 
	serveraddr.sin_port = htons((unsigned short)routerPort);
	if (bind(UDPfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){return -1;}
	//send INIT_REQUEST
	struct pkt_INIT_REQUEST initReq;
	initReq.router_id = htonl(routerID);
	if((sendTo = sendto(UDPfd, (struct pkt_INIT_REQUEST *) &initReq, sizeof(initReq), 0, (struct sockaddr *) &neaddr, sizeof(neaddr))) < 0){return -1;}
	
//	struct pkt_INIT_RESPONSE initRecv;
	if((recvFrom = recvfrom(UDPfd,(struct pkt_INIT_RESPONSE *) &initRecv, sizeof(initRecv), 0, NULL, NULL))<0){return -1;}


	ntoh_pkt_INIT_RESPONSE((struct pkt_INIT_RESPONSE *)&initRecv);
	InitRoutingTbl((struct pkt_INIT_RESPONSE *) &initRecv,routerID);
	total = time(NULL);
//	PrintRoutes(fptr, routerID);
//UPDATE
//	struct pkt_RT_Update RT_request;
/*	hton_pkt_RT_UPDATE((struct pkt_RT_UPDATE *)&RT_request);
	if((sendTo = sendto(UDPfd, (struct pkt_RT_Update *) &RT_request, sizeof(RT_request),0,(struct sockaddr *)&neaddr,sizeof(neaddr))<0)){return -1;}
	struct pkt_RT_Update rec_request;
	if((recvFrom = recvfrom(UDPfd, (struct pkt_RT_Update *) &rec_request, sizeof(rec_request),0,NULL,NULL))<0){return -1;}
	 ntoh_pkt_RT_UPDATE((struct pkt_RT_UPDATE *)&rec_request);
*/
	printf("here1\n");
//	FILE * fptr = NULL;
	char logfile[128];
//	char * logfile = malloc(sizeof("router")+sizeof(routerID)+sizeof(".log"));
	
//strcat(logfile,"router");
//	char buff[256];
	sprintf(logfile,"router%s.log",argv[1]);
//	strcat(logfile,argv[1]);
//	strcat(logfile,".log");
	fptr = fopen(logfile,"w");
	PrintRoutes(fptr,routerID);
	printf("here2\n");
	/*-----INIT THREADS-----*/
	pthread_t udpfd_polling_id;
	pthread_t update_timer_id; 
	int c;
	for(c = 0; c < MAX_ROUTERS; c++){
		realtime[c] = time(NULL);
	}
	checkcon = time(NULL);
	checkupdate = time(NULL);

	if(pthread_create(&udpfd_polling_id, NULL, udpfd_polling, NULL)){
		perror("Error creating thread for polling UDP file descriptor!");
		return EXIT_FAILURE;
	}
	if(pthread_create(&update_timer_id, NULL,update_timer, NULL)){
		perror("Error creating thread for timer updating!");
		return EXIT_FAILURE;
	}
	printf("here3\n");

/*-----WAIT FOR THREADS TO FINISH------*/
	pthread_join(udpfd_polling_id,NULL);
	pthread_join(update_timer_id,NULL);
	printf("here4\n");
	
 	fclose(fptr);
	
	
//Empty router update packet
//	struct pkt_RT_UPDATE * RT_UPDATE = malloc(sizeof(struct pkt_RT_UPDATE));
//	ConverTabletoPkt(PT_UPDATE,routerID);

	//PrintRoutes(logfile,routerID);
	return 0;
}
//example code for timer and threads
//https://gist.github.com/cirocosta/ab2b73b2e1bb34b83f52
