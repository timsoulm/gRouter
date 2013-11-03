/*
 *
 * OSPF
 *
 */

#include "ospf.h"
#include "message.h"
#include "grouter.h"
#include "gnet.h"
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>


void ICMPProcessPacket(gpacket_t *in_pkt)
{
	ip_packet_t *ip_pkt = (ip_packet_t *)in_pkt->data.data;
	int iphdrlen = ip_pkt->ip_hdr_len *4;
	ospfhdr_t *ospfhdr = (ospfhdr_t *)((uchar *)ip_pkt + iphdrlen);

	switch (ospfhdr->type)
	{
	case ICMP_ECHO_REQUEST:
		verbose(2, "[ICMPProcessPacket]:: ICMP processing for ECHO request");
		ICMPProcessEchoRequest(in_pkt);
		break;

	case ICMP_ECHO_REPLY:
		verbose(2, "[ICMPProcessPacket]:: ICMP processing for ECHO reply");
		ICMPProcessEchoReply(in_pkt);
		break;

	case ICMP_REDIRECT:
	case ICMP_SOURCE_QUENCH:
	case ICMP_TIMESTAMP:
	case ICMP_TIMESTAMPREPLY:
	case ICMP_INFO_REQUEST:
	case ICMP_INFO_REPLY:
		verbose(2, "[ICMPProcessPacket]:: ICMP processing for type %d not implemented ", icmphdr->type);
		break;
	}
}


int create_ls_update()
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

int create_common_header(ospf_header_common* header,
                         char type,
                         short length);
{
    header-> type = type;
    header-> msg_length = length;
    header-> source_ip_addr = NULL; // ???
}

int unpack_common_header(ospf_header_common* header)
{
    int status = 0;

    switch(header->type)
    case 1:
        unpack_hello_message();
        break;
    case 3:
        unpack_ls_request();
        break;
    case 4:
        unpack_ls_update();
        break;
    default:
        printf("Invalid OSPF Header Type\n");
        status = 1;

    return status;
}

int create_lsa_header()
{

}

int create_hello_packet(ospf_hello* hello_packet,
			int* neighbor_list_start)
{
	hello_packet-> network_mask = 0xFFFFFF00; //255.255.255.0
	hello_packet-> hello_interval = 10; //10 seconds
	hello_packet-> options = 0; //figure out later
	hello_packet-> priority = 0; //0
	hello_packet-> 40; //40 seconds
	hello_packet-> neighbor_list_start = neighbor_list_start;

}

void ospf_init()
{
	pthread_t tid;
	pthread_create(&tid, NULL, &hello_message_thread, NULL);
}

void *hello_message_thread(void *arg)
{
	int* neighbors;
	gpacket_t *out_pkt;
	ip_packet_t *ipkt;
	ospf_hello *hello_packet;
    while(1)
    {
    	neighbors = getInterfaceIPs();
    	out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
    	ipkt = (ip_packet_t *)(out_pkt->data.data);
    	ipkt->ip_hdr_len = 5;
    	hello_packet = (ospf_hello *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
    	create_hello_packet(hello_packet, neighbors);

    	//ip header+ospf header+ospf packet info+payload
    	IPProcessBcastPacket(out_pkt, ipkt->ip_hdr_len*4 + 44 + sizeof(neighbors));

    	//wait for 10 seconds before going again
    	sleep(10);
    }
    return 0;
}

