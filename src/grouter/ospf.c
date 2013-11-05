
/*
 * test
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
lsupdate_advertisement *ads_head;
int ospf_init_complete = 0;
int seq_num = 0; //Sequence number used in Router

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
    /*
    case OSPF_LS_UPDATE:
        verbose(2, "[OSPFProcessPacket]:: OSPF processing for Link Status Update");
        OSPProcessLSUpdate(in_pkt);
        break;
    */
    case OSPF_DBASE_DESCRIPTION:
    case OSPF_LS_REQUEST:
    case OSPF_STATUS_ACK:
        verbose(2, "[OSPFProcessPacket]:: OSPF processing for type %d not implemented ", ospfhdr->type);
        break;
    }
}

/*int create_ls_update()
{
    ospf_lsupdate packet;

    packet->ads = malloc(sizeof(NumOfInterfaces*ospf_ad))
    for(i = 0; i < NumOfInterfaces; i++)
    {
        ads[i]->Link = NetworkAddress;
        if(any2any)
        {

            ads{i}->data = routeraddress;
            ads[i]->type = 2;
        }
        else
        {
            ads{i}->data = NetworkMask;
            ads[i]->type = 3;
        }
    }
}
*/

/*

int create_lsa_header()
{

}
*/
void create_hello_packet(ospf_hello_pkt* hello_packet, short pkt_length, int src_ip)
{
    char tmpbuf[MAX_TMPBUF_LEN];
    hello_packet-> header.version = 2;
    hello_packet-> header.type = 1;
    hello_packet-> header.msg_length = htons(pkt_length);
    hello_packet-> header.source_ip_addr = htonl(src_ip);
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
    neighbor_list_head = NULL; //Changed to

    NumberOfInterfaces = getInterfaceIDsandIPs(&NeighborIDs,&NeighborIPs);
    verbose(1, "number of interfaces: %d",NumberOfInterfaces);
    
    for(i=0;i<NumberOfInterfaces;i++)
    {
        neighbor = (ospf_neighbor_t *) malloc(sizeof(ospf_neighbor_t));
        neighbor->interface_id = NeighborIDs[i];
        neighbor->source_ip = NeighborIPs[i];
        neighbor->destination_ip = 0;
        neighbor->alive = 0;
        neighbor->next = neighbor_list_head;
        neighbor_list_head = neighbor;
    }
    
	ospf_init_complete = 1;

	pthread_create(&tid, NULL, &hello_message_thread, NULL);
}

void create_lsupdate_packet(lsupdate_pkt_t* lsupdate_pkt, short pkt_length, int src_ip)
{
	ospfhdr_t common_header;
	ospf_header_lsa lsa_header;
	short zeros_in_pkt;
	short num_links;

	int i;
	lsupdate_advertisement* ad;
	uchar network;
	uchar network_mask = 0x00FFFFFF;
	char tmpbuf[MAX_TMPBUF_LEN];
	ospf_neighbor_t *curr;

	int NumKnownNeighbors = 0;

	lsupdate_pkt-> common_header.version = 4;
	lsupdate_pkt-> common_header.type = 1;
	lsupdate_pkt-> common_header.msg_length = htons(pkt_length);
	lsupdate_pkt-> common_header.source_ip_addr = htonl(src_ip);

	lsupdate_pkt-> lsa_header.ls_age    		= 0;
	lsupdate_pkt-> lsa_header.ls_type    	= 1;
	lsupdate_pkt-> lsa_header.ls_id      	= src_ip;
	lsupdate_pkt-> lsa_header.ad_router 		= src_ip;
	lsupdate_pkt-> lsa_header.ls_seq_num 	= seq_num;
	lsupdate_pkt-> lsa_header.ls_checksum 	= 0;

	for(curr = neighbor_list_head; curr != NULL; curr = curr->next)
	    {
	        if(curr -> alive == 1)
	        {
	            ad = (lsupdate_advertisement *) malloc(sizeof(lsupdate_advertisement));
	            network = curr->source_ip & network_mask; //Put network mask of source IP in 'network'
	            COPY_IP(ad->link_id, gHtonl(tmpbuf, network));
	            /*
	             * TODO: neighbor list must know if stub or A-A
	             * Must add this functionality to neighbor list
	             */
	            if(curr->is_stub == 1) //STUB
	            {
	            	COPY_IP(ad->local_data, gHtonl(tmpbuf, network));
	            	ad->link_type = STUB;
	            }
	            else //ANY_TO_ANY
	            {
	            	COPY_IP(ad->local_data, curr->source_ip);
	            	ad->link_type = ANY_TO_ANY;
	            }
	            ad->zeros_in_update = 0;
	            ad->zeros_in_update2 = 0;
	            ad->metrics = 1;
	            ad->next = ads_head;
	            ads_head = ad;
	            NumKnownNeighbors++;
	        }
	    }
	lsupdate_pkt->ads = *ads_head;
	lsupdate_pkt->lsa_header.ls_length = 4 + (NumKnownNeighbors*sizeof(lsupdate_advertisement)); //TODO: ?

	lsupdate_pkt->zeros_in_pkt = 0;
	lsupdate_pkt->num_links = NumKnownNeighbors;
}
void broadcast_lsupdate_packet(void)
{
    char* NeighborIPs;
    gpacket_t *out_pkt;
    ip_packet_t *ipkt;
    lsupdate_pkt_t *lsupdate_pkt;
	lsupdate_advertisement* ad;

    short PacketSize;
    ospf_neighbor_t *curr;
    ospf_neighbor_t *curr2;
    int NumberOfKnownNeighbours;
    int status, i;
    uchar bcast_ip[] = IP_BCAST_ADDR;
    int broadcast_int;
    uchar IPasCharArray[4];
    char tmpbuf[MAX_TMPBUF_LEN];

    verbose(1, "[Send_LSUpdate_Packet]:: Send is starting...");

    NumberOfKnownNeighbours = 0;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->alive == 1)
        {
            NumberOfKnownNeighbours++;
        }
    }
    PacketSize = sizeof(ospfhdr_t) + sizeof(ospf_header_lsa)+4+sizeof(lsupdate_advertisement)*NumberOfKnownNeighbours;

    lsupdate_pkt = malloc(sizeof(lsupdate_pkt_t));
    create_lsupdate_packet(lsupdate_pkt, PacketSize, curr->source_ip);

    for(curr=neighbor_list_head; curr != NULL; curr = curr->next) //Send to all live neighbors
    {
    	if(curr2->alive == 1)
		{
			i = 0;
			out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
			ipkt = (ip_packet_t *)(out_pkt->data.data);
			ipkt->ip_hdr_len = 5;
			out_pkt->frame.src_interface = 0;
			out_pkt->frame.dst_interface = curr->interface_id;


			duplicatePacket(out_pkt);
			status = IPOutgoingPacket(out_pkt, bcast_ip, PacketSize, 1, OSPF_PROTOCOL);
        }
    }
    seq_num++;
}
void OSPFSendHelloPacket(void)
{
    char* NeighborIPs;
    gpacket_t *out_pkt;
    ip_packet_t *ipkt;
    ospf_hello_pkt *hello_packet;
    short PacketSize;
    ospf_neighbor_t *curr; 
    ospf_neighbor_t *curr2;
    int NumberOfKnownNeighbours;
    int status, i;
    uchar bcast_ip[] = IP_BCAST_ADDR;
    int broadcast_int;
    uchar IPasCharArray[4];
    char tmpbuf[MAX_TMPBUF_LEN];
    
    verbose(1, "send hello packet starting");
    
    //ip header+ospf header+ospf packet info+payload

    NumberOfKnownNeighbours = 0;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->alive == 1)
        {
            NumberOfKnownNeighbours++;
        }
    }
    PacketSize = 44 + sizeof(int)*NumberOfKnownNeighbours;

    verbose(1,"list head interface id %d",neighbor_list_head->interface_id);

    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {	
	i = 0;
        out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
        ipkt = (ip_packet_t *)(out_pkt->data.data);
        ipkt->ip_hdr_len = 5;
	out_pkt->frame.src_interface = 0;
	out_pkt->frame.dst_interface = curr->interface_id;

        hello_packet = (ospf_hello_pkt *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
        NeighborIPs = (char*)hello_packet+44;

	for(curr2=neighbor_list_head; curr2 != NULL; curr2 = curr2->next)
    	{
        	if(curr2->alive == 1)
        	{
            		COPY_IP(NeighborIPs, gHtonl(tmpbuf, &(curr2->destination_ip)));
            		NeighborIPs+=4;
        	}
    	}

        create_hello_packet(hello_packet, PacketSize, curr->source_ip);
        status = IPOutgoingPacket(out_pkt, bcast_ip, PacketSize, 1, OSPF_PROTOCOL);
    }

}
void OSPFProcessHelloMsg(gpacket_t *in_pkt)
{
    ospf_neighbor_t *curr;
    char tmpbuf[MAX_TMPBUF_LEN];
    ip_packet_t *ip_pkt = (ip_packet_t *)in_pkt->data.data;

    //if linked list isnt initialized yet we dont want to accept any hello messages
    if(ospf_init_complete==0)
	return;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->interface_id == in_pkt->frame.src_interface)
        {
            COPY_IP(&(curr->destination_ip), gNtohl(tmpbuf, ip_pkt->ip_src));
	    //verbose(1,"ip_pkt->ip_src: %s",IP2Dot(tmpbuf,ip_pkt->ip_src));
	    //verbose(1,"curr->destination_ip: %s",IP2Dot(tmpbuf,&(curr->destination_ip)));
            if(curr->alive)
            {
                curr->time_since_hello = 0;
            } else {
                curr->alive = 1; //TODO: reset timer once we have one
                curr->time_since_hello = 0;
            }
            break;
        }
    }
}

