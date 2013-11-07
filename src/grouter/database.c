#include "database.h"
//Initialize Database

ls_database_t *database;
/*
* Debugging Code Below
*/
void createDBRecord(ls_database_t* entry, int router_id, int num_neighbors, int seq_num) //For testing purposes
{
	neighbor_t* head;
	neighbor_t* curr;
	head = NULL;

	int i;
	ls_database_t* tmp;

	for (i = 1; i <= num_neighbors; i++)
	{
		curr = (neighbor_t *)malloc(sizeof(neighbor_t));
		curr->link_id = 10-i;
		curr->router_id = 0;
		curr->next = head;
		head = curr;
	}
	curr = head;
	
	tmp = entry;
	tmp->router_id = router_id;
	tmp->seq_num = seq_num;
	tmp->neighbor_list = curr;
	tmp->next_record = NULL;
}
void printDatabase(void) //For debugging purposes
{
	ls_database_t* tmp_database_ptr;
	neighbor_t* neighbor_ptr;
	int i;
	for(tmp_database_ptr = database; tmp_database_ptr != NULL; tmp_database_ptr = tmp_database_ptr->next_record)
	{
		printf("Router ID: %d\n", tmp_database_ptr->router_id);
		i = 0;
		for(neighbor_ptr = tmp_database_ptr->neighbor_list; neighbor_ptr != NULL; neighbor_ptr = neighbor_ptr->next)
		{
			i++;
			printf("Neighbor %d: Link ID = %d, Router ID = %d \n", i, neighbor_ptr->link_id, neighbor_ptr->router_id);
		}
	}
}
// >> -------------------- End of Debugging Code ---------------------------- <<

/*
* Functions for use in OSPF.C
*/
int getNumLinks(void)
{
	int result = 0;
	neighbor_t* neighbor;
	for(neighbor = database->neighbor_list; neighbor != NULL; neighbor = neighbor->next)
	{
		result++;
	}
	return result;
}
int* getLinkIDs(void)
{
	int* result;
	neighbor_t* neighbor;
	int i;
	int length = getNumLinks();
	result = (int*)malloc(length*(sizeof(int)));
	neighbor = database->neighbor_list;
	printf("Made it to just before loop \n");
	for(i = 0; i < length; i++)
	{	
		result[i] = neighbor->link_id;
		neighbor = neighbor->next;
	}
	return result;

}
int* getStubs(void)
{
	int* result;
	neighbor_t* neighbor;
	int i;
	int length = getNumLinks();
	result = (int*)malloc(length*(sizeof(int)));
	neighbor = database->neighbor_list;
	printf("Made it to just before loop \n");
	for(i = 0; i < length; i++)
	{	
		result[i] = neighbor->is_stub;
		neighbor = neighbor->next;
	}
	return result;
}
/*
* Functions for Dijkstra's Aglorithm
*/
int getSizeOfDB(void)
{
	ls_database_t* tmp;
	int result = 0;
	for(tmp = database; tmp != NULL; tmp = tmp->next_record)
	{
		result++;
	}
	return result;
}
ls_database_t* getDatabase(void)
{
	ls_database_t* result;
	result = database;
	return result;
}
/*
* Database Functions
*/
neighbor_t* generateNeighborList(int* neighbors_linkids, int num_neighbors)
{
	neighbor_t* head;
	neighbor_t* curr;
	int i;
	head = NULL;
	for (i = 0; i < num_neighbors; i++)
	{
		if(neighbors_linkids[i]==NULL)
			continue;
		curr = (neighbor_t *)malloc(sizeof(neighbor_t));
		curr->link_id = neighbors_linkids[i];
		curr->router_id = 0;
		curr->next = head;
		head = curr;
	}
	curr = head;
	return curr;
}

void init_database(int router_id, int* neighbors_linkids, int num_neighbors)
{
	ls_database_t* tmp;
	neighbor_t* head; 
	head = generateNeighborList(neighbors_linkids, num_neighbors);
	
	tmp = (ls_database_t*)malloc(sizeof(ls_database_t));
	tmp->router_id = router_id;
	tmp->seq_num = 0;
	tmp->neighbor_list = head;
	tmp->next_record = NULL;
	database = tmp;
}

int checkSeqNum(int router_id) //WORKS
{
	int result;
	ls_database_t* tmp_database_ptr;
	for(tmp_database_ptr = database; tmp_database_ptr != NULL; tmp_database_ptr = tmp_database_ptr->next_record)
	{
		if(router_id == tmp_database_ptr->router_id)
		{
			return tmp_database_ptr->seq_num;
			break;
		}
	}
	return 0; //Returns 0 if no entry for it
}
int exists(int router_id) //WORKS
{
	int result;
	ls_database_t* tmp_database_ptr;
	for(tmp_database_ptr = database; tmp_database_ptr != NULL; tmp_database_ptr = tmp_database_ptr->next_record)
	{
		if(router_id == tmp_database_ptr->router_id)
		{
			return 1;//Exists
			break;
		}
	}
	return 0; //Doesn't exist
}
void addNewDBEntry(int router_id, int* neighbors, int num_neighbors, int seq_num)
{
	ls_database_t* tmp_database_ptr;
	ls_database_t* new_entry;
	neighbor_t* head; 

	new_entry = (ls_database_t*)malloc(sizeof(ls_database_t));

	head = generateNeighborList(neighbors, num_neighbors);

	new_entry->seq_num = seq_num;
	new_entry->next_record = NULL;
	new_entry->router_id = router_id;
	new_entry->neighbor_list = head;

	for(tmp_database_ptr = database; tmp_database_ptr != NULL; tmp_database_ptr = tmp_database_ptr->next_record)
	{
		if(tmp_database_ptr->next_record==NULL)
		{
			tmp_database_ptr->next_record = new_entry;
			break;
		}
	}

}
void updateDBEntry(int router_id, int* neighbors, int num_neighbors, int seq_num) //WORKS
{
	ls_database_t* tmp_database_ptr;
	neighbor_t* head; 
	head = generateNeighborList(neighbors, num_neighbors);


	for(tmp_database_ptr = database; tmp_database_ptr != NULL; tmp_database_ptr = tmp_database_ptr->next_record)
	{
		if(router_id == tmp_database_ptr->router_id)
		{
			free(tmp_database_ptr->neighbor_list);
			tmp_database_ptr->neighbor_list = head;
			break;
		}
	}
}
void updateDeadInterface(int link_id)
{
	ls_database_t* tmp_db_ptr;
	neighbor_t* curr;
	neighbor_t* prev;

	for(curr = database->neighbor_list; curr != NULL; curr = curr->next)
	{
		if(curr->link_id == link_id)
		{
			prev->next = curr-> next;
			free(curr);
			break;
		}
		prev = curr;
	}
}
void indexDatabase(void)
{
	ls_database_t* db_stable;
	ls_database_t* db_dynamic;

	neighbor_t* nei_stable;
	neighbor_t* nei_dynamic;



	for(db_stable = database; db_stable != NULL; db_stable = db_stable->next_record)
	{
		for(nei_stable = db_stable->neighbor_list; nei_stable != NULL; nei_stable = nei_stable->next)
		{
			for(db_dynamic = db_stable->next_record; db_dynamic != NULL; db_dynamic = db_dynamic->next_record)
			{
				for(nei_dynamic = db_dynamic->neighbor_list; nei_dynamic != NULL; nei_dynamic = nei_dynamic->next)
				{
					if(nei_stable->link_id == nei_dynamic->link_id)
					{
						nei_stable->router_id = db_dynamic->router_id;
						nei_dynamic->router_id = db_stable->router_id;
					}
				}
			}
		}
	}
}
int* find_all_linkids(void) //Return all the links in the network
{
	ls_database_t* tmp;

	neighbor_t* dynamic;

	int* array1;
	int i = 0;

	array1 = (int*)malloc(sizeof(int)*100);

	for(tmp = database; tmp != NULL; tmp= tmp->next_record)
	{
		for(dynamic = tmp->neighbor_list; dynamic!= NULL; dynamic = dynamic->next)
		{
			array1[i] = dynamic->link_id;
			i++;
		}
	}
	return array1;
}
int getSize(int* in_ary)
{
	int result = 0;
	int i = 0;
	while(in_ary[i] != NULL)
	{
		result++;
		i++;
	}
	return result;
}
int find_router_id(int link_id) //finds router ID that is connected to a specific link
{
	ls_database_t* tmp;
	neighbor_t* dynamic;

	for(tmp = database; tmp != NULL; tmp= tmp->next_record)
	{
		for(dynamic = tmp->neighbor_list; dynamic!= NULL; dynamic = dynamic->next)
		{
			if(dynamic->link_id == link_id)
				return dynamic->router_id;
		}
	}

}
int find_linkid_of_neighbor(int router_id) //find the link ID of the current router's neighbor
{
	ls_database_t* tmp;
	neighbor_t* dynamic;

	tmp = database;
	for(dynamic = tmp->neighbor_list; dynamic!= NULL; dynamic = dynamic->next)
	{
		if(dynamic->router_id == router_id)
			return dynamic->link_id;
	}
}
/*int main()
{
	
	//neighbor_t* neighbor;
	//neighbor = (neighbor_t*)malloc(sizeof(neighbor_t));
	//neighbor->link_id = 1;
	//neighbor->router_id = 0;
	//neighbor->next = NULL;
	//database = (ls_database_t*)malloc(sizeof(ls_database_t));
	//database->router_id = 1;
	//database->seq_num = 0;
	//database->neighbor_list = neighbor;


	ls_database_t* new_record = (ls_database_t*)malloc(sizeof(ls_database_t));
	ls_database_t* new_record2 = (ls_database_t*)malloc(sizeof(ls_database_t));
	ls_database_t* new_record3 = (ls_database_t*)malloc(sizeof(ls_database_t));
	
	int array[] = {1,2,3};
	init_database(1, array ,3);
	printf("\n");
	
	int array2[] = {1,2,3,7};
	addNewDBEntry(2, array2, 4, 4);

	int array3[] = {8,7};
	updateDBEntry(2, array3, 2, 5);
	//printDatabase();
	//printf("\n");
	
	printf("Database before index:  \n");
	updateDBEntry(2, array2, 4, 6);
	//printDatabase();
	//printf("\n");


	printf("Database after index:  \n");
	indexDatabase();
	//printDatabase();
	//printf("\n");

	printf("Database after update:  \n");
	addNewDBEntry(3, array3, 2, 6);
	//printDatabase();
	//printf("\n");

	
	printf("Database after second index:  \n");
	indexDatabase();
	//printDatabase();
	//printf("\n");

	printf("Database after update dead interface:  \n");
	updateDeadInterface(2);
	printDatabase();
	printf("\n");

	//printf("%d", (getDatabase())->next_record->router_id);
	//printf("\n");

	printf("Link IDs retrieved:  \n");
	int i;
	int* result;
	result = find_all_linkids();
	int max = getSize(result);
	
	for(i = 0; i < max; i++)
	{

		printf("%d \n", result[i]);
	}
	printf("\n");

	printf("Find router ID of link ID 8 = %d \n",find_router_id(8));
	printf("Find link ID of router 2 = %d \n", find_linkid_of_neighbor(2));
	/*createDBRecord(new_record, 2, 10, 1);
	createDBRecord(new_record2, 3, 5, 2);
	createDBRecord(new_record3, 2, 8, 4);
	database->next_record = new_record;
	//database->next_record->next_record = new_record2;
	printf("Initial Database before updating: \n");
	printDatabase();
	printf("\n");

	printf("Database after adding new entry:  \n");
	addNewDBEntry(new_record2);
	printDatabase();
	printf("\n");

	printf("Database after updating entry:  \n");
	updateDBEntry(new_record3);
	printDatabase();
	printf("\n");

	printf("Database after indexing  \n");

	
	printf("\n");*/
	
	//return 1;
//}
