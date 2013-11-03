
/*
 * test
 * OSPF
 *
 */

#include "ospf.h"
#include "message.h"
#include "grouter.h"
#include "gnet.h"
#include "ip.h"
#include "protocols.h"
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

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
int create_common_header(ospf_header_common* header,
                         char type,
                         short length)
{
    header-> type = type;
    header-> msg_length = length;
    header-> source_ip_addr = NULL; // ???
}

int create_lsa_header()
{

}
*/

int create_hello_packet(ospf_hello* hello_packet,
			int* neighbor_list_start)
{
	hello_packet-> network_mask = 0xFFFFFF00; //255.255.255.0
	hello_packet-> hello_interval = 10; //10 seconds
	hello_packet-> options = 0; //figure out later
	hello_packet-> priority = 0; //0
	hello_packet-> router_dead_interval = 40; //40 seconds
	hello_packet-> neighbor_list_start = neighbor_list_start;

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
	pthread_t tid;
	pthread_create(&tid, NULL, &hello_message_thread, NULL);
}

void OSPFSendHelloPacket()
{
    int* NeighborIPs;
    gpacket_t *out_pkt;
    ip_packet_t *ipkt;
    ospf_hello *hello_packet;
    int NumberOfInterfaces, PacketSize;

    out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
    ipkt = (ip_packet_t *)(out_pkt->data.data);
    ipkt->ip_hdr_len = 5;
    hello_packet = (ospf_hello *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
    NeighborIPs = (int*) hello_packet + 44;
    create_hello_packet(hello_packet, NeighborIPs);
    NumberOfInterfaces = getInterfaceIPs(NeighborIPs);
    //ip header+ospf header+ospf packet info+payload
    PacketSize = (ipkt->ip_hdr_len)*4 + 24*4 + sizeof(int)*NumberOfInterfaces;
    IPBroadcastPacket(out_pkt, PacketSize, OSPF_PROTOCOL);



}
void OSPFProcessHelloMsg(gpacket_t *in_pkt)
{

}

