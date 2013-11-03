typedef struct ospf_header_common{
	//char version;
	char type;
	short msg_length;
	int source_ip_addr;
	//int area_id;
	//short checksum;
	//short authentication_type;
	//int* authentication_array_start;
} ospf_header_common; 

typedef struct ospf_hello{
	ospf_header_common header;
	int network_mask;
	short hello_interval;
	char options;
	char priority;
	int router_dead_interval;
	//int desginated_router_ip;
	//int desginated_router_backup;
	int* neighbor_list_start;
} ospf_header_hello;

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
	ospf_header_common common_header;	
	int ls_type;
	int link_id;
	int ad_router_ip;
} ospf_lsrequest;

typedef struct ospf_lsupdate{
	ospf_header_common common_header;	
	ospf_header_lsa lsa_header;
	short num_links;
	ospf_ad* ads;
} ospf_lsupdate;

typedef struct ospf_ad{
	int link_id;
	int local_data;
	char link_type;
} ospf_ad;


