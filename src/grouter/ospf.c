
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

    sleep(15);
    while(1)
    {
        OSPFSendHelloPacket();
        sleep(5);
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
    
    for(i=0;i<NumberOfInterfaces;i++)
    {
        neighbor = (ospf_neighbor_t *) malloc(sizeof(ospf_neighbor_t));
        neighbor->interface_id = NeighborIDs[i];
        neighbor->source_ip = NeighborIPs[i];
        neighbor->alive = 0;
        neighbor->next = neighbor_list_head;
        neighbor_list_head = neighbor;
    }
    
	ospf_init_complete = 1;

	pthread_create(&tid, NULL, &hello_message_thread, NULL);
}

void OSPFSendHelloPacket(void)
{
    int* NeighborIPs;
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
    
    verbose(1, "send hello packet starting");
    
    //ip header+ospf header+ospf packet info+payload

    NumberOfKnownNeighbours = 0;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->alive == 1)
        {
            //NeighborIPs[NumberOfKnownNeighbours] = curr->destination_ip;
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
        NeighborIPs = (int*)(uchar*)hello_packet+44;

	for(curr2=neighbor_list_head; curr2 != NULL; curr2 = curr2->next)
    	{
        	if(curr2->alive == 1)
        	{
            		NeighborIPs[i] = curr2->destination_ip;
            		i++;
        	}
    	}


	//verbose(1,"Source:%d, Destination:%d, Interface:%d, Alive:%d",curr->
	
        create_hello_packet(hello_packet, PacketSize, curr->source_ip);
        status = IPOutgoingPacket(out_pkt, bcast_ip, PacketSize, 1, OSPF_PROTOCOL);
    }

}
void OSPFProcessHelloMsg(gpacket_t *in_pkt)
{
    ospf_neighbor_t *curr;

    //if linked list isnt initialized yet we dont want to accept any hello messages
    if(ospf_init_complete==0)
	return;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->interface_id == in_pkt->frame.src_interface)
        {
            curr->destination_ip = (int)*(in_pkt->frame.src_ip_addr);
            if(curr->alive)
            {
                //reset_timer();
            } else {
                curr->alive = 1; //TODO: reset timer once we have one
                //start_timer();
            }
            break;
        }
    }
}

