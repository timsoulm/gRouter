
/*
 *
 * OSPF
 *
 */

#include "ospf.h"
#include "grouter.h"
#include "gnet.h"
#include "ip.h"
#include "protocols.h"
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include "database.h"

ospf_neighbor_t *neighbor_list_head;
MyRouter_t MyRouter;

int ospf_init_complete = 0;

void OSPFProcessPacket(gpacket_t *in_pkt)
{
    ip_packet_t *ip_pkt = (ip_packet_t *)in_pkt->data.data;
    int iphdrlen = ip_pkt->ip_hdr_len *4;
    ospfhdr_t *ospfhdr = (ospfhdr_t *)((uchar *)ip_pkt + iphdrlen);

    switch (ospfhdr->type)
    {
    case OSPF_HELLO:
        verbose(2, "[OSPFProcessPacket]:: OSPF processing for HELLO MSG");
        OSPFProcessHelloMsg(in_pkt);
        break;

    case OSPF_LS_UPDATE:
        verbose(2, "[OSPFProcessPacket]:: OSPF processing for Link Status Update");
        OSPFProcessLSUpdate(in_pkt);
        break;

    case OSPF_DBASE_DESCRIPTION:
    case OSPF_LS_REQUEST:
    case OSPF_STATUS_ACK:
        verbose(2, "[OSPFProcessPacket]:: OSPF processing for type %d not implemented ", ospfhdr->type);
        break;
    }
}

void create_hello_packet(ospf_hello_pkt* hello_packet, short pkt_length)
{
    char tmpbuf[MAX_TMPBUF_LEN];

    hello_packet-> header.version = 2;
    hello_packet-> header.type = OSPF_HELLO;
    hello_packet-> header.msg_length = htons(pkt_length);
    hello_packet-> header.area_id = 0;
    hello_packet-> header.checksum = 0;
    hello_packet-> header.authentication_type = 0;
    hello_packet-> header.authentication_array_start = 0;

	hello_packet-> network_mask = 0x00FFFFFF; //255.255.255.0
	hello_packet-> hello_interval = htons(10); //10 seconds
	hello_packet-> options = 0; //figure out later
	hello_packet-> priority = 0; //0
	hello_packet-> router_dead_interval = htonl(40); //40 seconds
}

void *hello_message_thread(void *arg)
{
	ospf_neighbor_t *curr;

    sleep(15);
    while(1)
    {
        OSPFSendHelloPacket();
        sleep(5);
        for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
        {
       		(curr->time_since_hello)++;
       		if((curr->time_since_hello == 5) && (curr->type == ANY_TO_ANY)) {
       			curr->alive = 0;
       			updateDeadInterface((curr->source_ip) & 0xFFFFFF00);
                verbose(1, "MESSAGETHREAD: Link Dead ///////////////////////////////");
       			OSPFSendLSUpdate(); //Inform Routers that link is down and Update database
            }
        }
        printDatabase();
    }
    return 0;
}

void ospf_init()
{
    int i;
    int NumberOfInterfaces;
    int *NeighborIDs;
    int *NeighborIPs;
    ospf_neighbor_t *neighbor;
    int *link_id_array;
    pthread_t tid;

    verbose(1, "opsf_init starting");
    neighbor_list_head = NULL;

    NumberOfInterfaces = getInterfaceIDsandIPs(&NeighborIDs,&NeighborIPs);
    verbose(1, "number of interfaces: %d",NumberOfInterfaces);

    MyRouter.id = FindMin(NeighborIPs, NumberOfInterfaces);
    MyRouter.ls_seq_num = 0;
    MyRouter.num_of_interfaces = NumberOfInterfaces;

    link_id_array = (int *)malloc(sizeof(int)*NumberOfInterfaces);

    for(i=0;i<NumberOfInterfaces;i++)
    {
        neighbor = (ospf_neighbor_t *) malloc(sizeof(ospf_neighbor_t));
        neighbor->interface_id = NeighborIDs[i];
        neighbor->source_ip = NeighborIPs[i];
        neighbor->destination_ip = 0;
        neighbor->link_id = NeighborIPs[i] & 0xFFFFFF00;
        link_id_array[i] = neighbor->link_id;
        neighbor->alive = 0;
        neighbor->next = neighbor_list_head;
        neighbor_list_head = neighbor;
        neighbor->type = STUB;
    }

	init_database(MyRouter.id, link_id_array, NumberOfInterfaces);
	ospf_init_complete = 1;
    verbose(1, "INITIAL DATABASE PRINTED BELOW ==========>\n");
    printDatabase();

	pthread_create(&tid, NULL, &hello_message_thread, NULL);
}

void create_lsupdate_packet(lsupdate_pkt_t* lsupdate_pkt, short pkt_length, int ls_ID, short NumOfLinks)
{
	lsupdate_pkt-> common_header.version = 2;
	lsupdate_pkt-> common_header.type = OSPF_LS_UPDATE;
	lsupdate_pkt-> common_header.msg_length = htons(pkt_length);
    lsupdate_pkt-> common_header.area_id = 0;
    lsupdate_pkt-> common_header.checksum = 0;
    lsupdate_pkt-> common_header.authentication_type = 0;
    lsupdate_pkt-> common_header.authentication_array_start = 0;

	lsupdate_pkt-> lsa_header.ls_age    	= 0;
	lsupdate_pkt-> lsa_header.ls_type    	= htons(1);
	lsupdate_pkt-> lsa_header.ls_id      	= htonl(ls_ID);
	lsupdate_pkt-> lsa_header.ad_router 	= htonl(ls_ID);
	lsupdate_pkt-> lsa_header.ls_seq_num 	= htonl(MyRouter.ls_seq_num);
	lsupdate_pkt-> lsa_header.ls_checksum 	= 0;
	lsupdate_pkt-> lsa_header.ls_length     = htons(pkt_length-sizeof(ospfhdr_t));
	lsupdate_pkt-> zeros_in_pkt             = 0;
	lsupdate_pkt-> num_links                = htons(NumOfLinks);
}

void OSPFSendLSUpdate(void)
{
    gpacket_t *out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
    ip_packet_t *ipkt;
    lsupdate_pkt_t *lsupdate_pkt;
    lsupdate_entry_t *entry;
    short PacketSize;
    ospf_neighbor_t *curr;
    int NumberOfAliveNeighbours;
    char tmpbuf[MAX_TMPBUF_LEN];
    char *is_stub_array;
    int *link_id_array;
    uchar net_mask[] = {0x00, 0xFF, 0xFF, 0xFF};

    (MyRouter.ls_seq_num)++;
    updateMySeqNum();
    //verbose(1, "[Send_LSUpdate_Packet]:: Send is starting...");
    NumberOfAliveNeighbours = GetNumberOfAliveNeighbours();
    PacketSize = sizeof(lsupdate_pkt_t) + sizeof(lsupdate_entry_t)*(NumberOfAliveNeighbours);

    ipkt = (ip_packet_t *)(out_pkt->data.data);
    ipkt->ip_hdr_len = 5;
    out_pkt->frame.src_interface = 0;

    lsupdate_pkt = (lsupdate_pkt_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
    create_lsupdate_packet(lsupdate_pkt, PacketSize, MyRouter.id, NumberOfAliveNeighbours);

    entry = (lsupdate_entry_t *) ((uchar *)ipkt + (ipkt->ip_hdr_len)*4 + sizeof(lsupdate_pkt_t));

    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        COPY_IP(&(entry->link_id), gHtonl(tmpbuf, &(curr->link_id)));
        if((curr->type == ANY_TO_ANY) && (curr->alive == 1)) {
            COPY_IP(&(entry->link_data), gHtonl(tmpbuf, &(curr->source_ip)));
        } else if(curr->type == STUB) {
            COPY_IP(&(entry->link_data), gHtonl(tmpbuf, net_mask));
        }
        entry->link_type = curr->type;
        entry->metric = htons(1);
        entry = (lsupdate_entry_t *)((uchar *)entry + sizeof(lsupdate_entry_t));
    }

    OSPFbroadcastPacket(out_pkt, PacketSize);
}

void OSPFSendHelloPacket(void)
{
    char* NeighborIPs;
    gpacket_t *out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
    ip_packet_t *ipkt;
    ospf_hello_pkt *hello_packet;
    short PacketSize;
    ospf_neighbor_t *curr;
    int NumberOfKnownNeighbours;
    char tmpbuf[MAX_TMPBUF_LEN];

    //verbose(1, "send hello packet starting");

    NumberOfKnownNeighbours = GetNumberOfKnownNeighbours();
    PacketSize = 44 + sizeof(int)*NumberOfKnownNeighbours;

    ipkt = (ip_packet_t *)(out_pkt->data.data);
    ipkt->ip_hdr_len = 5;
    out_pkt->frame.src_interface = 0;
    hello_packet = (ospf_hello_pkt *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
    NeighborIPs = (char*)hello_packet+44;

    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->alive == 1)
        {
            COPY_IP(NeighborIPs, gHtonl(tmpbuf, &(curr->destination_ip)));
            NeighborIPs+=4;
        }
    }

    create_hello_packet(hello_packet, PacketSize);
    OSPFbroadcastPacket(out_pkt, PacketSize);
}

void OSPFProcessHelloMsg(gpacket_t *in_pkt)
{
    ospf_neighbor_t *curr;
    char tmpbuf[MAX_TMPBUF_LEN];
    ip_packet_t *ip_pkt = (ip_packet_t *)in_pkt->data.data;

    //if linked list isnt initialized yet we dont want to accept any hello messages
    if(ospf_init_complete==0) return;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->interface_id == in_pkt->frame.src_interface)
        {
            COPY_IP(&(curr->destination_ip), gNtohl(tmpbuf, ip_pkt->ip_src));
            curr->type = ANY_TO_ANY;
	    //verbose(1,"ip_pkt->ip_src: %s",IP2Dot(tmpbuf,ip_pkt->ip_src));
	    //verbose(1,"curr->destination_ip: %s",IP2Dot(tmpbuf,&(curr->destination_ip)));
            if(curr->alive==1)
            {
                curr->time_since_hello = 0;
            } else if (curr->alive == 0) {
                curr->alive = 1;
                curr->time_since_hello = 0;
                updateLiveInterface(((curr->source_ip) & 0xFFFFFF00));
                verbose(1, "OSPFProcessHello: Link Went alive ====================");
                OSPFSendLSUpdate(); //Inform Routers that link is down and Update database
            }
            break;
        }
    }
    free(in_pkt);
}

void OSPFProcessLSUpdate(gpacket_t *in_pkt)
{
    ospf_neighbor_t *curr;
    char tmpbuf[MAX_TMPBUF_LEN];
    ip_packet_t *ipkt = (ip_packet_t *)(in_pkt->data.data);
    lsupdate_pkt_t *lsupdate_pkt;
    int incoming_router_id, incoming_seq_num;
    short incoming_num_of_links;
    int *link_id_array;
    lsupdate_entry_t *entry;
    int PacketSize;
    char *link_type_array;
    int i;
    //if linked list isnt initialized yet we dont want to accept LSUpdates
    if(ospf_init_complete==0)return;

    lsupdate_pkt = (lsupdate_pkt_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4);

    COPY_IP(&incoming_router_id, gNtohl(tmpbuf, &(lsupdate_pkt->lsa_header.ls_id)));
    COPY_IP(&incoming_seq_num, gNtohl(tmpbuf, &(lsupdate_pkt->lsa_header.ls_seq_num)));

    verbose(1, "incoming router_id= %x, incoming_seq_num = %d, database_seq_num = %d", incoming_router_id, incoming_seq_num, checkSeqNum(incoming_router_id));
    if(EntryExists(incoming_router_id) == 1)
    {
        if(checkSeqNum(incoming_router_id) >= incoming_seq_num)
        {
            // Drop Packet
            free(in_pkt);
            return;
        }
    }

    incoming_num_of_links = ntohs(lsupdate_pkt->num_links);
    link_id_array = (int *)malloc(sizeof(int)*incoming_num_of_links);
    link_type_array = (char *)malloc(sizeof(char)*incoming_num_of_links);


    entry = (lsupdate_entry_t *) ((uchar *)ipkt + (ipkt->ip_hdr_len)*4 + sizeof(lsupdate_pkt_t));

    for(i = 0; i <incoming_num_of_links; i++)
    {
        COPY_IP(&link_id_array[i] , gNtohl(tmpbuf, &(entry->link_id)));
        link_type_array[i] = entry->link_type;
        entry = (lsupdate_entry_t *)((uchar *)entry + sizeof(lsupdate_entry_t));
    }

    if(EntryExists(incoming_router_id) == 1)
    {
        updateDBEntry(incoming_router_id, link_id_array, link_type_array, incoming_num_of_links, incoming_seq_num);
    } else if(EntryExists(incoming_router_id) == 0)
    {
        addNewDBEntry(incoming_router_id, link_id_array, link_type_array, incoming_num_of_links, incoming_seq_num);
    }
    PacketSize = ntohs(lsupdate_pkt->common_header.msg_length);

    IPBroadcastPacket(in_pkt, PacketSize, OSPF_PROTOCOL);
}

void OSPFbroadcastPacket(gpacket_t *out_pkt, int PacketSize)
{
    gpacket_t *cppkt;
    ospfhdr_t *header;
    ospf_neighbor_t *curr;
    ip_packet_t *ipkt;
    uchar bcast_ip[] = IP_BCAST_ADDR;
	int status;

    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        cppkt = duplicatePacket(out_pkt);
        ipkt = (ip_packet_t *)(cppkt->data.data);
        header = (ospfhdr_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
        cppkt->frame.dst_interface = curr->interface_id;
        header->source_ip_addr = htonl(curr->source_ip);
        status = IPOutgoingPacket(cppkt, bcast_ip, PacketSize, 1, OSPF_PROTOCOL);
    }

}

int GetNumberOfKnownNeighbours(void)
{

    int NumberOfKnownNeighbours = 0;
    ospf_neighbor_t *curr;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->alive == 1)
        {
            NumberOfKnownNeighbours++;
        }
    }
    return NumberOfKnownNeighbours;
}

int FindMin(int* array, int size)
{
    int minimum, i;

    minimum = array[0];

    for ( i = 1 ; i < size ; i++ )
    {
        if ( array[i] < minimum )
        {
           minimum = array[i];
        }
    }

    return minimum;
}

int GetNumberOfAliveNeighbours(void)
{
    int NumberOfAliveNeighbours = 0;
    ospf_neighbor_t *curr;

    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
    	switch(curr->type) {
    	case(STUB):
    		NumberOfAliveNeighbours++;
    		break;
        case(ANY_TO_ANY):
        	if(curr->alive == 1) NumberOfAliveNeighbours++;
        	break;
        }

    }
    return NumberOfAliveNeighbours;
}

