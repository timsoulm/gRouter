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
