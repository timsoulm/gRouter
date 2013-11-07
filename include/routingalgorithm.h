#include "database.h"

//route_entry_t route_tbl[MAX_ROUTES];

int getIndexFromRouterID(int routerID, int size);
void createMap(int **cost_matrix,ls_database_t *list_head,int sizeOfList);
int allSelected(int *selected, int size);
void calculateNextHops(int **cost_matrix, int size, int *next_hops, int my_router_id);
void addRoutingEntries(int *next_hops);

