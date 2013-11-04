
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

ospf_neighbor_t *neighbor_list_head = NULL;


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
int create_hello_packet(ospf_hello_pkt* hello_packet, short pkt_length, int src_ip)
{
    hello_packet-> header.version = 2;
    hello_packet-> header.msg_length = pkt_length;
    hello_packet-> header.source_ip_addr = src_ip;
	hello_packet-> network_mask = 0xFFFFFF00; //255.255.255.0
	hello_packet-> hello_interval = 10; //10 seconds
	hello_packet-> options = 0; //figure out later
	hello_packet-> priority = 0; //0
	hello_packet-> router_dead_interval = 40; //40 seconds
}

void *hello_message_thread(void *arg)
{


    while(1)
    {
        OSPFSendHelloPacket();
        //wait for 10 seconds before going again
        sleep(10);
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

    NumberOfInterfaces = getInterfaceIDsandIPs(NeighborIDs,NeighborIPs);

    for(i=0;i<NumberOfInterfaces;i++)
    {
        neighbor = (ospf_neighbor_t *) malloc(sizeof(ospf_neighbor_t));
        neighbor->interface_id = NeighborIDs[i];
        neighbor->source_ip = NeighborIPs[i];
        neighbor->alive = 0;

        neighbor->next = neighbor_list_head;
        neighbor_list_head = neighbor;
    }


	pthread_t tid;
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
    int NumberOfKnownNeighbours;

    
    //ip header+ospf header+ospf packet info+payload

    NumberOfKnownNeighbours = 0;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->alive == 1)
        {
            NeighborIPs[NumberOfKnownNeighbours] = curr->destination_ip;
            NumberOfKnownNeighbours++;
        }
        
        curr = curr->next;
    }
    PacketSize = 44 + sizeof(int)*NumberOfKnownNeighbours;


    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
        ipkt = (ip_packet_t *)(out_pkt->data.data);
        ipkt->ip_hdr_len = 5;
        hello_packet = (ospf_hello_pkt *)((uchar *)ipkt + ipkt->ip_hdr_len*4);

        NeighborIPs = (int*)(uchar*)hello_packet+44;

        create_hello_packet(hello_packet, PacketSize, curr->source_ip);
        IPOutgoingPacket(out_pkt, IP_BCAST_ADDR, PacketSize, 1, OSPF_PROTOCOL);
    }

}
void OSPFProcessHelloMsg(gpacket_t *in_pkt)
{
    ospf_neighbor_t *curr;
    for(curr=neighbor_list_head; curr != NULL; curr = curr->next)
    {
        if(curr->interface_id == in_pkt->frame.src_interface)
        {
            curr->destination_ip = in_pkt->frame.src_ip_addr;
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

