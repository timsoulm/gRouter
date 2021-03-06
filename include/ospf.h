#include "message.h"

#define OSPF_HELLO              1
#define OSPF_DBASE_DESCRIPTION  2
#define OSPF_LS_REQUEST         3
#define OSPF_LS_UPDATE          4
#define OSPF_STATUS_ACK         5
#define ANY_TO_ANY 				2
#define STUB					3

typedef struct ospfhdr_t{
	char version;
	char type;
	short msg_length;
	int source_ip_addr;
	int area_id;
	short checksum;
	short authentication_type;
	long long authentication_array_start;
} ospfhdr_t;

struct ospf_neighbor_i{
	int source_ip;
	int destination_ip;
	int link_id;
	int interface_id;
	int alive;
	int time_since_hello;
	char type;
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

typedef struct ospfhdr_lsa{ //Reviewed
	short ls_age;
	short ls_type;
    int ls_id;
	int ad_router;
	int ls_seq_num;
	short ls_checksum;
	short ls_length;
} ospfhdr_lsa;


typedef struct ospf_lsrequest{
	ospfhdr_t common_header;
	int ls_type;
	int link_id;
	int ad_router_ip;
} ospf_lsrequest;


typedef struct lsupdate_entry_t{
	int link_id; //Network Address
	int link_data; //Router address for any-to-any, Network mask for stub
	char link_type; //Any-to-any or STUB
	char zeros_in_update[5];
	short metric;
} lsupdate_entry_t;

typedef struct lsupdate_pkt_t{
	ospfhdr_t common_header;
	ospfhdr_lsa lsa_header;
	short zeros_in_pkt;
	short num_links;
} lsupdate_pkt_t;

typedef struct MyRouter_t {
    int id;
    int ls_seq_num;
    short num_of_interfaces;
} MyRouter_t;

void OSPFProcessPacket(gpacket_t *in_pkt);
void OSPFProcessHelloMsg(gpacket_t *in_pkt);
void OSPFProcessLSUpdate(gpacket_t *in_pkt);
void create_lsupdate_packet(lsupdate_pkt_t* lsupdate_pkt, short pkt_length, int ls_ID, short NumOfLinks);
void broadcast_lsupdate_packet(void);
void OSPFSendHelloPacket(void);
void create_hello_packet(ospf_hello_pkt* hello_packet, short pkt_length);
int FindMin(int* array, int size);
void OSPFbroadcastPacket(gpacket_t *out_pkt, int PacketSize);
void OSPFSendLSUpdate(void);
