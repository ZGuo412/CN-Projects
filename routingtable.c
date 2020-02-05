#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;

//Update Entry
void entryUpdate(int x, int dest_id, int next_hop, int cost){
	routingTable[x].dest_id = dest_id;
	routingTable[x].next_hop = next_hop;
	routingTable[x].cost = cost;
	return;
}

////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	/* ----- YOUR CODE HERE ----- */
	//number of router
	NumRoutes = InitResponse->no_nbr + 1;
	int i;	
	for( i = 0;i < InitResponse->no_nbr; i++){
		entryUpdate(i,InitResponse->nbrcost[i].nbr,InitResponse->nbrcost[i].nbr,InitResponse->nbrcost[i].cost);
		routingTable[i].path_len = 2;
//		routingTable[i].path[MAX_PATH_LEN] = {-1};
		routingTable[i].path[0] = myID;
		routingTable[i].path[1] = routingTable[i].dest_id;
	}
	//self route
	entryUpdate(InitResponse->no_nbr,myID,myID,0);
	routingTable[InitResponse->no_nbr].path_len = 1;
//	routingTable[InitResponse->no_nbr].path[MAX_PATH_LEN] = {-1};
	routingTable[InitResponse->no_nbr].path[0] = myID;
	routingTable[InitResponse->no_nbr].path[1] = myID;
	return;
}

int check(int myID, struct route_entry route){
	int i;
	for(i = 0; i < route.path_len;i++){
		if(myID == route.path[i]){
			return 0;
		}
	}
	return 1;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	 /*----- YOUR CODE HERE -----*/
  int i;
  int changed = 0;
  for(i = 0; i< RecvdUpdatePacket-> no_routes; i++){
    int total_cost = RecvdUpdatePacket->route[i].cost + costToNbr; //cost(x,z) + cost(z,y);
    if(total_cost >= INFINITY){
      total_cost = INFINITY;
    }
    int des_id = RecvdUpdatePacket->route[i].dest_id;
    int j;
    int flag =0;
    for(j = 0;j<NumRoutes;j++){
      if(routingTable[j].dest_id == des_id){//forced update
	if(routingTable[j].next_hop == RecvdUpdatePacket->sender_id){
	  if(routingTable[j].cost != total_cost){
	    changed = 1;
	    flag = 1;
	  }
	}
	else if(total_cost<routingTable[j].cost &&  check(myID, RecvdUpdatePacket->route[i])){     //split horizen
	    changed = 1;
	    flag = 1;
	}
	//neither forced update or split horizen. changed should be zero.
	
	if(flag == 1){  ///which means I have to change and I need to change my cost and path here.
	 

	    routingTable[j].next_hop = RecvdUpdatePacket->sender_id;
	    routingTable[j].cost = total_cost;
	    changed =1;
	    flag = 1;
			if(RecvdUpdatePacket->route[i].path_len == MAX_PATH_LEN){
					routingTable[j].cost = INFINITY;					
			}
			else{
	    	routingTable[j].path_len = RecvdUpdatePacket->route[i].path_len + 1;
	//routingTable[count].path_len++;
	    	routingTable[j].path[0] = myID;
	    	int x;
	    	for(x = 0; x < RecvdUpdatePacket->route[i].path_len; x++){
	      	routingTable[j].path[1+x] = RecvdUpdatePacket->route[i].path[x];
  								
	    	}
			}
	}
	break;
      }
      
    }
    if(j==NumRoutes){
	changed = 1;
	flag = 1;
	entryUpdate(NumRoutes,des_id,RecvdUpdatePacket->sender_id,total_cost);
	routingTable[j].path_len = RecvdUpdatePacket->route[i].path_len+1;
	routingTable[j].path[0] = myID;
	int x;
	for(x = 0; x < RecvdUpdatePacket->route[i].path_len; x++){
	     routingTable[j].path[1+x] = RecvdUpdatePacket->route[i].path[x];
	}
	NumRoutes++;
    }
  }
  /*int changed = 0;
	//	bool updated = false;
	int i;
	for(i = 0;i < RecvdUpdatePacket-> no_routes; i++){//received update
		int total_cost = RecvdUpdatePacket->route[i].cost + costToNbr; //cost(x,z) + cost(z,y);
		int des_id = RecvdUpdatePacket->route[i].dest_id;
		int count = 0;
		while(des_id != routingTable[count].dest_id){
			count++;
			if(count == NumRoutes){
				break;
			}
		}
		//cant find des_id == routingtable.des_id
		if(count == NumRoutes){
			changed = 1;
			entryUpdate(NumRoutes,des_id,RecvdUpdatePacket->sender_id,total_cost);
			routingTable[count].path_len++;
			routingTable[count].path[routingTable[count].path_len] = des_id;
			NumRoutes++;
		}
		else{
		  	  	if(routingTable[count].cost == total_cost){
				  int a;
				  int same = 1;
				  if(routingTable[count].path_len - RecvdUpdatePacket->route[i].path_len == 1){
				    for(a = 0; a < RecvdUpdatePacket->route[i].path_len; a++){
				      if(routingTable[count].path[a + 1] != RecvdUpdatePacket->route[i].path[a]){
					same = 0;
				      }
				    }
				    if(same){
				      // changed = 0;
				      break;
				    }
				  }
				  }
			if(routingTable[count].next_hop == RecvdUpdatePacket->sender_id || (routingTable[count].cost > total_cost && check(myID, RecvdUpdatePacket->route[i])))
			{


				if(total_cost < INFINITY){
				  routingTable[count].cost = total_cost;
					//	changed = 1;
				}
				else if(routingTable[count].cost < total_cost){
				  //	changed = 0;
					break;
				}
				
				routingTable[count].next_hop = RecvdUpdatePacket->sender_id;
				routingTable[count].path_len = RecvdUpdatePacket->route[i].path_len + 1;
				//routingTable[count].path_len++;
				routingTable[count].path[0] = myID;
				int j;
				for(j = 0; j < RecvdUpdatePacket->route[i].path_len; j++){
					routingTable[count].path[1+j] = RecvdUpdatePacket->route[i].path[j];
				}								
				changed = 1;
			}
		}
		

	}
*/

	return changed;
}



////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	/* ----- YOUR CODE HERE ----- */
  	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;
	int i;
	for(i = 0; i < NumRoutes; i++)
	{
		//UpdatePacketToSend->no_routes++;
		UpdatePacketToSend->route[i] = routingTable[i];
	}
	return;
}


////////////////////////////////////////////////////////////////
//It is highly recommended that you do not change this function!
void PrintRoutes (FILE* Logfile, int myID){
	/* ----- PRINT ALL ROUTES TO LOG FILE ----- */
	int i;
	int j;
	for(i = 0; i < NumRoutes; i++){
		fprintf(Logfile, "<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

		/* ----- PRINT PATH VECTOR ----- */
		for(j = 1; j < routingTable[i].path_len; j++){
			fprintf(Logfile, " -> R%d", routingTable[i].path[j]);	
		}
		fprintf(Logfile, ", Cost: %d\n", routingTable[i].cost);
	}
	fprintf(Logfile, "\n");
	fflush(Logfile);
}


////////////////////////////////////////////////////////////////
void UninstallRoutesOnNbrDeath(int DeadNbr){
	/* ----- YOUR CODE HERE ----- */
 		int i;
	 	for(i = 0; i < NumRoutes; i++){
		if(routingTable[i].next_hop == DeadNbr){
			routingTable[i].cost = INFINITY;
		}
	}
	return;
}
