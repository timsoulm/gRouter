#include "message.h"

#define OSPF_HELLO              1
#define OSPF_DBASE_DESCRIPTION  2
#define OSPF_LS_REQUEST         3
#define OSPF_LS_UPDATE          4
#define OSPF_STATUS_ACK         5

typedef struct ospfhdr_t{
	char version;
	char type;
	short msg_length;
	int source_ip_addr;
	int area_id;
	short checksum;
	short authentication_type;
	long authentication_array_start;
} ospfhdr_t;

struct ospf_neighbor_i{
	int source_ip;
	int destination_ip;
	int interface_id;
	int alive;
	struct ospf_neighbor_i *next;
};

typedef struct ospf_neighbor_i ospf_neighbor_t;

typedef struct ospf_hello_pkt{
	ospfhdr_t header;
	int network_mask;
	short hello_interval;
	char options;
	char priority;
	int router_dead_interval;
	int desginated_router_ip;
	int desginated_router_backup_ip;
} ospf_hello_pkt;

typedef struct ospf_header_lsa{
	short ls_age;
	short ls_type;
	int ls_id;
	int ad_router;
	int ls_seq_num;
	short ls_checksum;
	short ls_length;
} ospf_header_lsa;

typedef struct ospf_lsrequest{
	ospfhdr_t common_header;	
	int ls_type;
	int link_id;
	int ad_router_ip;
} ospf_lsrequest;

typedef struct ospf_ad{
	int link_id;
	int local_data;
	char link_type;
} ospf_ad;

typedef struct ospf_lsupdate{
	ospfhdr_t common_header;	
	ospf_header_lsa lsa_header;
	short num_links;
	ospf_ad* ads;
} ospf_lsupdate;

void OSPFProcessPacket(gpacket_t *in_pkt);
void OSPFProcessHelloMsg(gpacket_t *in_pkt);
void OSPFProcessLSUpdate(gpacket_t *in_pkt);
void OSPFSendHelloPacket(void);
