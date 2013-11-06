
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
       		if(curr->time_since_hello >= 5)
       			curr->alive = 0;
       			//UpdateRoutingTable();
       			//SendLSUpdate(); //Inform Routers that link is down and Update database
        }
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
    pthread_t tid;

    verbose(1, "opsf_init starting");
    neighbor_list_head = NULL;

    NumberOfInterfaces = getInterfaceIDsandIPs(&NeighborIDs,&NeighborIPs);
    verbose(1, "number of interfaces: %d",NumberOfInterfaces);

    MyRouter.id = FindMin(NeighborIPs, NumberOfinterfaces);
    MyRouter.ls_seq_num = 0;

    for(i=0;i<NumberOfInterfaces;i++)
    {
        neighbor = (ospf_neighbor_t *) malloc(sizeof(ospf_neighbor_t));
        neighbor->interface_id = NeighborIDs[i];
        neighbor->source_ip = NeighborIPs[i];
        neighbor->destination_ip = 0;
        neighbor->link_id = NeighborIPs[i] & 0xFFFFFF00;
        neighbor->alive = 0;
        neighbor->next = neighbor_list_head;
        neighbor_list_head = neighbor;
        neighbor->type = STUB;
    }

// TODO (Mido#1#): Initialize Database ...
//
    //InitDbase();
	ospf_init_complete = 1;

	pthread_create(&tid, NULL, &hello_message_thread, NULL);
}

void create_lsupdate_packet(lsupdate_pkt_t* lsupdate_pkt, short pkt_length, int ls_ID, short NumOfLinks)
{
	ospfhdr_t common_header;
	ospf_header_lsa lsa_header;
	short zeros_in_pkt;
	short num_links;

	int i;
	lsupdate_entry* ad;
	uchar network;
	uchar network_mask = 0x00FFFFFF;
	char tmpbuf[MAX_TMPBUF_LEN];
	ospf_neighbor_t *curr;

	int NumKnownNeighbors = 0;

	lsupdate_pkt-> common_header.version = 2;
	lsupdate_pkt-> common_header.type = OSPF_LS_UPDATE;
	lsupdate_pkt-> common_header.msg_length = htons(pkt_length);

	lsupdate_pkt-> lsa_header.ls_age    	= 0;
	lsupdate_pkt-> lsa_header.ls_type    	= htons(1);
	lsupdate_pkt-> lsa_header.ls_id      	= htonl(ls_ID);
	lsupdate_pkt-> lsa_header.ad_router 	= htonl(ls_ID);
	lsupdate_pkt-> lsa_header.ls_seq_num 	= htonl(seq_num);
	lsupdate_pkt-> lsa_header.ls_checksum 	= 0;
	lsupdate_pkt-> lsa_header.ls_length     = htons(pkt_length-sizeof(ospfhdr_t));
	lsupdate_pkt-> zeros_in_pkt             = 0;
	lsupdate_pkt-> num_links                = NumOfLinks;
}

void OSPFSendLSUpdate(void)
{
    gpacket_t out_pkt;
    ip_packet_t *ipkt;
    lsupdate_pkt_t *lsupdate_pkt;
	lsupdate_entry* entry;
    short PacketSize;
    ospf_neighbor_t *curr;
    int NumberOfKnownNeighbours;
    char tmpbuf[MAX_TMPBUF_LEN];
    char *is_stub_array;
    int *link_id_array;
    uchar[] net_mask = {0xFF, 0xFF, 0xFF, 0x00};

    verbose(1, "[Send_LSUpdate_Packet]:: Send is starting...");

    NumberOfKnownNeighbours = GetNumberOfKnownNeighbours();
    PacketSize = sizeof(ospfhdr_t) + sizeof(ospfhdr_lsa)+4+sizeof(lsupdate_entry)*NumberOfKnownNeighbours;

    ipkt = (ip_packet_t *)(out_pkt.data.data);
    ipkt->ip_hdr_len = 5;
    out_pkt.frame.src_interface = 0;

    lsupdate_pkt = (lsupdate_pkt_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
    create_lsupdate_packet(lsupdate_pkt, PacketSize, GetMyLSID(), NumberOfKnownNeighbours);

    entry = ipkt + (ipkt->ip_hdr_len)*4 + sizeof(ospfhdr_t) + sizeof(ospfhdr_lsa);


    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        COPY_IP(entry->link_id, gHtonl(tmpbuf, &(curr->link_id)));
        if(curr->type == STUB) {
            COPY_IP(entry->link_data, gHtonl(tmpbuf, &(curr->source_ip)));
        } else if(curr->type == ANY_TO_ANY) {
            COPY_IP(entry->link_data, gHtonl(tmpbuf, net_mask));
        }
        entry->link_type = htons(curr->type);
        entry->metric = htons(1);
        entry = entry + sizeof(entry);
    }

    OSPFbroadcastPacket(&out_pkt, PacketSize);
    (MyRouter.ls_seq_num)++;
}

void OSPFSendHelloPacket(void)
{
    char* NeighborIPs;
    gpacket_t out_pkt;
    ip_packet_t *ipkt;
    ospf_hello_pkt *hello_packet;
    short PacketSize;
    ospf_neighbor_t *curr;
    int NumberOfKnownNeighbours;
    char tmpbuf[MAX_TMPBUF_LEN];

    verbose(1, "send hello packet starting");

    NumberOfKnownNeighbours = GetNumberOfKnownNeighbours();
    PacketSize = 44 + sizeof(int)*NumberOfKnownNeighbours;

    ipkt = (ip_packet_t *)(out_pkt.data.data);
    ipkt->ip_hdr_len = 5;
    out_pkt.frame.src_interface = 0;
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
    OSPFbroadcastPacket(&out_pkt, PacketSize);
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
            if(curr->alive)
            {
                curr->time_since_hello = 0;
            } else {
                curr->alive = 1;
                curr->time_since_hello = 0;
                //UpdateRoutingTable();
                //OSPFSendLSUpdate(); //Inform Routers that link is down and Update database
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
    ip_packet_t *ip_pkt = (ip_packet_t *)in_pkt->data.data;
    lsupdate_pkt_t *lsupdate_pkt = (lsupdate_pkt_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
    int incoming_router_id, incoming_seq_num, incoming_num_of_links;
    int *link_id_array;
    lsupdate_entry entry;
    int PacketSize;

    //if linked list isnt initialized yet we dont want to accept LSUpdates
    if(ospf_init_complete==0)return;

    incoming_router_id = gNtohl(tmpbuf, &(lsupdate_pkt->lsa_header.ls_id));
    incoming_seq_num = gNtohl(tmpbuf, &(lsupdate_pkt->lsa_header.ls_seq_num));
/*
    if(EntryExists(incoming_router_id) == 1)
    {
        if(SeqNum(incoming_router_id) >= incoming_seq_num)
        {
            // Drop Packet
            free(in_pkt);
            return;
        }
    }

    incoming_num_of_links = gNtohl(tmpbuf, &(lsupdate_pkt->num_links));
    link_id_array = malloc(sizeof(int)*incoming_num_of_links);
    link_type_array = malloc(sizeof(char)*incoming_num_of_links);

    entry = ipkt + (ipkt->ip_hdr_len)*4 + sizeof(ospfhdr_t) + sizeof(ospfhdr_lsa);

    for(i = 0; i <num_of_links; i++)
    {
        COPY_IP(&link_id_array[i] , gNtohl(tmpbuf, entry->link_id));
        is_stub_array[i] = entry->link_type;
        entry = entry + sizeof(entry);
    }
/*
    dbase.router_id = incoming_router_id;
    dbase.seq_num = incoming_seq_num;
    dbase.id_array = link_id_array;
    dbase.type_array = link_type_array;
    dbase.array_length = incoming_num_of_links;

    if(EntryExists(incoming_router_id) == 1)
    {
        UpdateEntry(dbase);
    }
    } else if(EntryExists(incoming_router_id) == 0)
    {
        CreateEntry(dbase);
    }
*/  PacketSize = ntohs(lsupdate_pkt->common_header.msg_length);
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
    NumberOfKnownNeighbours = 0;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->alive == 1)
        {
            NumberOfKnownNeighbours++;
        }
    }
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
