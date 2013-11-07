#include "routingalgorithm.h"
#include "ospf.h"
#include "gnet.h"
#include "routetable.h"
#include "database.h"

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
			int routerID = getIndexFromRouterID(curr->router_id,sizeOfList);
			int neighborID = getIndexFromRouterID(curr2->router_id,sizeOfList);
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
		next_hops[i] = IndexToRouterID[i];
	}
	
}

void addRoutingEntries(int *next_hops, int size, ls_database_t *list_head)
{
	int i;
	uchar *nwork;
	uchar *nmask;
	uchar *nhop;
	int interface;
	int next_hop_router_id;
	int destination_router_id;
	int next_hop_link_id;
	int destination_link_id;
	ls_database_t *curr;

	for(i = 0; i<size; i++)
	{
		//lookup destination router id from table
		destination_router_id = IndexToRouterID[i];
		//lookup next hop router id from table
		next_hop_router_id = next_hops[i];
		
		for(curr=list_head; curr!= NULL; curr = curr->next_record)
		{
			if(curr->router_id == destination_router_id)
			{
				destination_link_id = curr->link_id;
			}
			if(curr->router_id == next_hop_link_id)
			{
				next_hop_link_id = curr->link_id;
			}
		}
		
		nmask = (uchar*)malloc(sizeof(char)*4);
		nmask = {0xFF,0xFF,0xFF,0x00};
		nwork = (uchar*)&destination_link_id;
		nhop = (uchar*)&next_hop_link_id;
		interface = findInterfaceByIP(nwork);
		
		addRouteEntry(route_tbl,nwork,nmask,nhop,interface);
	}

}
