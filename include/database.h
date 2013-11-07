#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>

typedef struct neighbor_t{
	int link_id;
	int router_id;
	int is_stub;
	struct neighbor_t *next;
} neighbor_t;

typedef struct ls_database_t{
	int router_id;
	int seq_num;
	neighbor_t *neighbor_list;
	struct ls_database_t *next_record;
} ls_database_t;
/*
typedef struct linkid_storage{
	int link_id;
	struct linkid_storage* next;
} linkid_storage;
*/
void createDBRecord(ls_database_t* entry, int router_id, int num_neighbors, int seq_num);
void printDatabase(void);
int getNumLinks(void);
int* getLinkIDs(void);
int* getStubs(void);
int getSizeOfDB(void);
neighbor_t* generateNeighborList(int* neighbors_linkids, char* is_stubs, int num_neighbors);
void init_database(int router_id, int* neighbors_linkids, int num_neighbors);
int checkSeqNum(int router_id);
int EntryExists(int router_id);
void addNewDBEntry(int router_id, int* neighbors, char* is_stubs, int num_neighbors, int seq_num);
void updateDBEntry(int router_id, int* neighbors, char* is_stubs, int num_neighbors, int seq_num);
void updateDeadInterface(int link_id);
void indexDatabase(void);
int* find_all_linkids(void);
int getSize(int* in_ary);
int find_router_id(int link_id);
int find_linkid_of_neighbor(int router_id);



