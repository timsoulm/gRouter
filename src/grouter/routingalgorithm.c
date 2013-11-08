#include "routingalgorithm.h"
#include "gnet.h"
#include "routetable.h"

int *IndexToRouterID;

int getIndexFromRouterID(int routerID, int size)
{
	int i;
	for(i=0;i<size;i++)
	{
		if(IndexToRouterID[i] == routerID)
		{
			return i;
		}
	}
	return -1;
}

void createMap(int **cost_matrix,ls_database_t *list_head,int sizeOfList)
{
	ls_database_t *curr;
	neighbor_t *curr2;
	int i, j;
	int neighborID;
	int routerID;


	printRouteTable(route_tbl);
	IndexToRouterID = (int*)malloc(sizeof(int)*sizeOfList);

	i = 0;
	for(curr=list_head; curr!= NULL; curr = curr->next_record)
	{
		IndexToRouterID[i] = curr->router_id;
		i++;
	}
	// traverse list and store values in matrix
	for(curr=list_head; curr!= NULL; curr = curr->next_record)
	{
		
		for(curr2=curr->neighbor_list; curr2!=NULL; curr2 = curr2->next)
		{
			routerID = getIndexFromRouterID(curr->router_id,sizeOfList);
			if(curr2->router_id == 0)
			{
				neighborID = routerID;
			} else {
				neighborID = getIndexFromRouterID(curr2->router_id,sizeOfList);
			}
			cost_matrix[routerID][neighborID] = 1;
		}
	}
	
}

int allSelected(int *selected, int size)
{
	int i;
	for(i=0;i<size;i++){
		if(selected[i]==0){
			return 0;
		}
	}
	return 1;
}

void calculateNextHops(int **cost_matrix, int size, int *next_hops, int my_router_id)
{
	int *preced;
	int *distance;
	int *selected;
	int current=my_router_id;
	int i,k,dc,smalldist,newdist,j;
	int INFINITE = 9999;

	selected = (int*)malloc(sizeof(int)*size);
	memset(selected, 0, size*sizeof(int));
	preced = (int*)malloc(sizeof(int)*size);
	distance = (int*)malloc(sizeof(int)*size);
	
	
	for(i=0;i<size;i++){
		distance[i]=INFINITE;
		preced[i]=0;
	}
	selected[current]=1;
	distance[my_router_id]=0;
	current=my_router_id;
	//while there are still unselected nodes
	

	while(!allSelected(selected, size))
	{
		smalldist=INFINITE;
		dc=distance[current];
		for(i=0;i<size;i++)
		{
			if(selected[i]==0)
			{                                           
				newdist=dc+cost_matrix[current][i];
				if(newdist<distance[i])
				{
					distance[i]=newdist;
					preced[i]=current;

				}
				if(distance[i]<smalldist)
				{
					smalldist=distance[i];
					k=i;
				}
			}
		}
		current=k;
		selected[current]=1;
	}
	
	
	for(i=0;i<size;i++)
	{
		int currentIndex = preced[i];
		next_hops[i] = my_router_id;
		
		while(currentIndex != my_router_id)
		{
			if(preced[currentIndex] == my_router_id)
			{
				next_hops[i] = currentIndex;
			}
			currentIndex = preced[currentIndex];
		}
	}
	
	for(i=0;i<size;i++)
	{
		if(next_hops[i] == my_router_id)
		{
			next_hops[i] = i;
		}
	}
	next_hops[my_router_id] = my_router_id;
		

	// Convert indexes to Router IDs
	for(i=0;i<size;i++)
	{
		next_hops[i] = IndexToRouterID[next_hops[i]];
	}
	
}

void addRoutingEntries(int *next_hops)
{
	int i;
	int index;
	char *nwork;
	char nmask[] = {0x00,0xFF,0xFF,0xFF};
	char *nhop;
	int interface;
	int next_hop_router_id;
	int destination_router_id;
	int next_hop_link_id;
	int destination_link_id;
	ls_database_t *curr;
	
	int *linkIDs = find_all_linkids();
	int size = getSize(linkIDs);
	int sizeOfRouterIDtable;

	for(i=0; i<size; i++)
	{
		// get router id from link id
		destination_router_id = find_router_id(linkIDs[i]);
		
		// look up next hop router id
		sizeOfRouterIDtable = getSizeOfDB();

		index = getIndexFromRouterID(destination_router_id, sizeOfRouterIDtable);

		next_hop_router_id = next_hops[index];


		// look up next hop link id
		next_hop_link_id = find_linkid_of_neighbor(next_hop_router_id);

		// look up destination link id
		destination_link_id = linkIDs[i];

		nwork = (char*)&destination_link_id;
		nhop = (char*)&next_hop_link_id;
		interface = findInterfaceByIP(nhop);
		
		addRouteEntry(route_tbl,nwork,nmask,nhop,interface);

	}
	printRouteTable(route_tbl);

}
