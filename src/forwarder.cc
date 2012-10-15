/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#include "forwarder.hh"

CLICK_DECLS

ForwardingEntry::ForwardingEntry() {
    src = NULL;
    dst = NULL;
    src_ip = NULL;
    dst_ip = NULL;
    LID = NULL;
}

ForwardingEntry::~ForwardingEntry() {
    if (src != NULL) {
        delete src;
    }
    if (dst != NULL) {
        delete dst;
    }
    if (src_ip != NULL) {
        delete src_ip;
    }
    if (dst_ip != NULL) {
        delete dst_ip;
    }
    if (LID != NULL) {
        delete LID;
    }
}

Forwarder::Forwarder() {

}

Forwarder::~Forwarder() {
    click_chatter("Forwarder: destroyed!");
}

int Forwarder::configure(Vector<String> &conf, ErrorHandler *errh) {
    int port;
    int reverse_proto;
    int reverse_notify_proto ;
    int reverse_reqpub_proto ;
    int reverse_reqrouter_proto ;
    int reverse_responserouter_proto ;
    int reverse_cr2sub_proto ;
    gc = (GlobalConf *) cp_element(conf[0], this);
    _id = 0;
    click_chatter("*****************************************************FORWARDER CONFIGURATION*****************************************************");
    click_chatter("Forwarder: internal LID: %s", gc->iLID.to_string().c_str());
    cp_integer(String("0x080a"), 16, &reverse_proto);
    cp_integer(String("0x0901"), 16, &reverse_notify_proto) ;
    cp_integer(String("0x0902"), 16, &reverse_reqpub_proto) ;
    cp_integer(String("0x0903"), 16, &reverse_reqrouter_proto) ;
    cp_integer(String("0x0904"), 16, &reverse_responserouter_proto) ;
    cp_integer(String("0x0905"), 16, &reverse_cr2sub_proto) ;
    proto_type = htons(reverse_proto) ;
    notify_type = htons(reverse_notify_proto) ;
    req_pub_type = htons(reverse_reqpub_proto) ;
    req_router_type = htons(reverse_reqrouter_proto) ;
    response_router_type = htons(reverse_responserouter_proto) ;
    cr2sub_type = htons(reverse_cr2sub_proto) ;
    if (gc->use_mac == true) {
        cp_integer(conf[1], &number_of_links);
        click_chatter("Forwarder: Number of Links: %d", number_of_links);
        for (int i = 0; i < number_of_links; i++) {
            cp_integer(conf[2 + 4 * i], &port);
            EtherAddress * src = new EtherAddress();
            EtherAddress * dst = new EtherAddress();
            cp_ethernet_address(conf[3 + 4 * i], src, this);
            cp_ethernet_address(conf[4 + 4 * i], dst, this);
            ForwardingEntry *fe = new ForwardingEntry();
            fe->src = src;
            fe->dst = dst;
            fe->port = port;
            fe->LID = new BABitvector(FID_LEN * 8);
            for (int j = 0; j < conf[5 + 4 * i].length(); j++) {
                if (conf[5 + 4 * i].at(j) == '1') {
                    (*fe->LID)[conf[5 + 4 * i].length() - j - 1] = true;
                } else {
                    (*fe->LID)[conf[5 + 4 * i].length() - j - 1] = false;
                }
            }
            fwTable.push_back(fe);
            if (port != 0) {
                click_chatter("Forwarder: Added forwarding entry: port %d - source MAC: %s - destination MAC: %s - LID: %s", fe->port, fe->src->unparse().c_str(), fe->dst->unparse().c_str(), fe->LID->to_string().c_str());
            } else {
                click_chatter("Forwarder: Added forwarding entry for the internal LINK ID: %s", fe->LID->to_string().c_str());
            }
        }
    } else {
        cp_integer(conf[1], &number_of_links);
        click_chatter("Forwarder: Number of Links: %d", number_of_links);
        for (int i = 0; i < number_of_links; i++) {
            cp_integer(conf[2 + 4 * i], &port);
            IPAddress * src_ip = new IPAddress();
            IPAddress * dst_ip = new IPAddress();
            cp_ip_address(conf[3 + 4 * i], src_ip, this);
            cp_ip_address(conf[4 + 4 * i], dst_ip, this);
            ForwardingEntry *fe = new ForwardingEntry();
            fe->src_ip = src_ip;
            fe->dst_ip = dst_ip;
            fe->port = port;
            fe->LID = new BABitvector(FID_LEN * 8);
            for (int j = 0; j < conf[5 + 4 * i].length(); j++) {
                if (conf[5 + 4 * i].at(j) == '1') {
                    (*fe->LID)[conf[5 + 4 * i].length() - j - 1] = true;
                } else {
                    (*fe->LID)[conf[5 + 4 * i].length() - j - 1] = false;
                }
            }
            fwTable.push_back(fe);
            click_chatter("Forwarder: Added forwarding entry: port %d - source IP: %s - destination IP: %s - LID: %s", fe->port, fe->src_ip->unparse().c_str(), fe->dst_ip->unparse().c_str(), fe->LID->to_string().c_str());
        }
    }
    click_chatter("*********************************************************************************************************************************");
    //click_chatter("Forwarder: Configured!");
    return 0;
}

int Forwarder::initialize(ErrorHandler *errh) {
    //click_chatter("Forwarder: Initialized!");
    data_sent_byte = 0 ;
    data_sent_GB = 0 ;
    data_forward_byte = 0 ;
    data_forward_GB = 0 ;

    oneGB = 1073741824 ;
    return 0;
}

void Forwarder::cleanup(CleanupStage stage) {
    if (stage >= CLEANUP_CONFIGURED) {
        for (int i = 0; i < number_of_links; i++) {
            ForwardingEntry *fe = fwTable.at(i);
            delete fe;
        }
    }
    FILE *ft ;
    if( (ft = fopen("/home/forwarder_cinc.dat", "w+")) == NULL )
        click_chatter("forwarder fopen error") ;
    fprintf(ft, "data_sent_byte: %d\ndata_sent_GB: %d\ndata_forward_byte: %d\ndata_forward_GB: %d\n",
            data_sent_byte, data_sent_GB, data_forward_byte, data_forward_GB) ;
    fclose(ft) ;
    click_chatter("Forwarder: Cleaned Up!");
}

void Forwarder::push(int in_port, Packet *p) {
    WritablePacket *newPacket;
    WritablePacket *payload = NULL;
    ForwardingEntry *fe;
    Vector<ForwardingEntry *> out_links;
    BABitvector FID(FID_LEN * 8);
    BABitvector andVector(FID_LEN * 8);
    Vector<ForwardingEntry *>::iterator out_links_it;
    int counter = 1;
    bool pushLocally = false;
    click_ip *ip;
    click_udp *udp;
    if (in_port == 0 || in_port == 2 || in_port == 4 || in_port == 6 || in_port == 8 || in_port == 10) {
        memcpy(FID._data, p->data(), FID_LEN);
        /*Check all entries in my forwarding table and forward appropriately*/
        for (int i = 0; i < fwTable.size(); i++) {
            fe = fwTable[i];
            andVector = (FID)&(*fe->LID);
            if (andVector == (*fe->LID)) {
                out_links.push_back(fe);
            }
        }
        andVector = FID & gc->iLID ;
        if(in_port == 2 && andVector == gc->iLID)
        {
        	if(out_links.size() != 0)
        	{
        		WritablePacket* temppacket = p->clone()->uniqueify() ;
        		temppacket->pull(FID_LEN) ;
        		output(2).push(temppacket) ;
        	}
        	else
        	{
        		p->pull(FID_LEN) ;
        		output(2).push(p) ;
        	}
        }
        if(in_port == 4 && andVector == gc->iLID)
        {
        	if(out_links.size() != 0)
        	{
        		WritablePacket* temppacket = p->clone()->uniqueify() ;
        		temppacket->pull(FID_LEN) ;
        		output(3).push(temppacket) ;
        	}
        	else
        	{
        		p->pull(FID_LEN) ;
        		output(3).push(p) ;
        	}
        }
        if(in_port == 6 && andVector == gc->iLID)
        {
        	if(out_links.size() != 0)
        	{
        	    //this case only happens when the publisher and cache router are the same node
        		WritablePacket* temppacket = p->clone()->uniqueify() ;
        		temppacket->pull(FID_LEN) ;
        		output(4).push(temppacket) ;
        	}
        	else
        	{
        	    p->pull(FID_LEN) ;
        		output(4).push(p) ;
        	}
        }
        if (out_links.size() == 0) {
            /*I can get here when an app or a click element did publish_data with a specific FID
             *Note that I never check if I can push back the packet above if it matches my iLID
             * the upper elements should check before pushing*/
            p->kill();
        }
        for (out_links_it = out_links.begin(); out_links_it != out_links.end(); out_links_it++) {
            if (counter == out_links.size()) {
                payload = p->uniqueify();
            } else {
                payload = p->clone()->uniqueify();
            }
            fe = *out_links_it;
            if (gc->use_mac) {
                newPacket = payload->push_mac_header(14);
                /*prepare the mac header*/
                /*destination MAC*/
                memcpy(newPacket->data(), fe->dst->data(), MAC_LEN);
                /*source MAC*/
                memcpy(newPacket->data() + MAC_LEN, fe->src->data(), MAC_LEN);
                if(in_port == 0)
                {
                    /*protocol type 0x080a*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN, &proto_type, 2);
                }
                if(in_port == 2)
                {
                    /*protocol type 0x0901*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN, &notify_type, 2);
                }
                if(in_port == 4)
                {
                    /*protocol type 0x0902*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN, &req_pub_type, 2);
                }
                if(in_port == 6)
                {
                    /*protocol type 0x0903*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN, &req_router_type, 2);
                }
                if(in_port == 8)
                {
                    /*protocol type 0x0904*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN, &response_router_type, 2);
                }
                if(in_port == 10)
                {
                    /*protocol type 0x0905*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN, &cr2sub_type, 2);
                }
                /*push the packet to the appropriate ToDevice Element*/
                data_sent_byte += newPacket->length() ;
                if( data_sent_byte >= oneGB ){
                    data_sent_byte = data_sent_byte - oneGB ;
                    data_sent_GB++ ;
                }
                output(fe->port).push(newPacket);
            } else {
                newPacket = payload->push(sizeof (click_udp) + sizeof (click_ip));
                ip = reinterpret_cast<click_ip *> (newPacket->data());
                udp = reinterpret_cast<click_udp *> (ip + 1);
                // set up IP header
                ip->ip_v = 4;
                ip->ip_hl = sizeof (click_ip) >> 2;
                ip->ip_len = htons(newPacket->length());
                ip->ip_id = htons(_id.fetch_and_add(1));
                ip->ip_p = IP_PROTO_UDP;
                ip->ip_src = fe->src_ip->in_addr();
                ip->ip_dst = fe->dst_ip->in_addr();
                ip->ip_tos = 0;
                ip->ip_off = 0;
                ip->ip_ttl = 250;
                ip->ip_sum = 0;
                ip->ip_sum = click_in_cksum((unsigned char *) ip, sizeof (click_ip));
                newPacket->set_ip_header(ip, sizeof (click_ip));
                // set up UDP header
                udp->uh_sport = htons(55555);
                udp->uh_dport = htons(55555);
                uint16_t len = newPacket->length() - sizeof (click_ip);
                udp->uh_ulen = htons(len);
                udp->uh_sum = 0;
                unsigned csum = click_in_cksum((unsigned char *) udp, len);
                udp->uh_sum = click_in_cksum_pseudohdr(csum, ip, len);
                output(fe->port).push(newPacket);
            }
            counter++;
        }
    } else if (in_port == 1 || in_port == 3 || in_port == 5 || in_port == 7 || in_port == 9 || in_port == 11) {
        /**a packet has been pushed by the underlying network.**/
        /*check if it needs to be forwarded*/
        if (gc->use_mac) {
            memcpy(FID._data, p->data() + 14, FID_LEN);
        } else {
            memcpy(FID._data, p->data() + 28, FID_LEN);
        }
        BABitvector testFID(FID);
        BABitvector reverse_FID(FID_LEN*8) ;//KC: modify the sub2cr fid
        EtherAddress reverse_dst ;//KC: modify the sub2cr fid
        EtherAddress reverse_src ;//KC: modify the sub2cr fid
        unsigned char noofhop = 0 ;
        if(in_port == 11)
        {//get the reverse src and dst
            memcpy(reverse_src.data(), p->data(), MAC_LEN) ;
            memcpy(reverse_dst.data(), p->data()+MAC_LEN, MAC_LEN) ;
            memcpy(&noofhop, p->data()+14+FID_LEN+sizeof(unsigned char)+NODEID_LEN, sizeof(noofhop)) ;
            memcpy(reverse_FID._data, p->data()+14+FID_LEN+sizeof(unsigned char)+NODEID_LEN+sizeof(noofhop), FID_LEN) ;
        }
        testFID.negate();
        if (!testFID.zero()) {
            /*Check all entries in my forwarding table and forward appropriately*/
            for (int i = 0; i < fwTable.size(); i++) {
                fe = fwTable[i];
                andVector = (FID)&(*fe->LID);
                if (andVector == (*fe->LID)) {
                    if (gc->use_mac) {
                        EtherAddress src(p->data() + MAC_LEN);
                        EtherAddress dst(p->data());
                        if ((src.unparse().compare(fe->dst->unparse()) == 0) && (dst.unparse().compare(fe->src->unparse()) == 0)) {
                            click_chatter("MAC: a loop from positive..I am not forwarding to the interface I received the packet from");
                            continue;
                        }
                    } else {
                        click_ip *ip = reinterpret_cast<click_ip *> ((unsigned char *)p->data());
                        if ((ip->ip_src.s_addr == fe->dst_ip->in_addr().s_addr) && (ip->ip_dst.s_addr == fe->src_ip->in_addr().s_addr)) {
                            click_chatter("IP: a loop from positive..I am not forwarding to the interface I received the packet from");
                            continue;
                        }
                    }
                    out_links.push_back(fe);
                }
                if(in_port == 11)//KC: find the reverse fid
                {
                    if(((*fe->src) == reverse_src) && ((*fe->dst) == reverse_dst) && (reverse_FID != gc->iLID))
                    {
                        reverse_FID |= (*fe->LID) ;
                    }
                }
            }
        } else {
            /*all bits were 1 - probably from a link_broadcast strategy--do not forward*/
        }
        /*check if the packet must be pushed locally*/
        andVector = FID & gc->iLID;
        if (andVector == gc->iLID) {
            pushLocally = true;
        }
        if (!testFID.zero()) {
            for (out_links_it = out_links.begin(); out_links_it != out_links.end(); out_links_it++) {
                if ( ((counter == out_links.size()) || (in_port == 11)) && (pushLocally == false) ) {
                    payload = p->uniqueify();
                } else {
                    payload = p->clone()->uniqueify();
                }
                fe = *out_links_it;
                if (gc->use_mac) {
                    /*prepare the mac header*/
                    /*destination MAC*/
                    memcpy(payload->data(), fe->dst->data(), MAC_LEN);
                    /*source MAC*/
                    memcpy(payload->data() + MAC_LEN, fe->src->data(), MAC_LEN);
                    data_forward_byte += payload->length() ;
                    if(in_port == 11)
                    {
                        noofhop++ ;
                        memcpy(payload->data()+14+sizeof(unsigned char), &noofhop, sizeof(noofhop)) ;
                        memcpy(payload->data()+14+sizeof(unsigned char)+sizeof(noofhop), reverse_FID._data, FID_LEN) ;
                        break ;
                        //pay attention here, the cache2sub response should only be forwarded once
                    }
                    if(data_forward_byte >= oneGB)
                    {
                        data_forward_byte = data_forward_byte - oneGB ;
                        data_forward_GB++ ;
                    }
                    /*push the packet to the appropriate ToDevice Element*/
                    output(fe->port).push(payload);
                } else {
                    click_ip *ip = reinterpret_cast<click_ip *> (payload->data());
                    ip->ip_src = fe->src_ip->in_addr();
                    ip->ip_dst = fe->dst_ip->in_addr();
                    ip->ip_tos = 0;
                    ip->ip_off = 0;
                    ip->ip_ttl = 250;
                    ip->ip_sum = 0;
                    ip->ip_sum = click_in_cksum((unsigned char *) ip, sizeof (click_ip));
                    click_udp *udp = reinterpret_cast<click_udp *> (ip + 1);
                    //uint16_t len = udp->uh_ulen;
                    uint16_t len = payload->length() - sizeof (click_ip);
                    udp->uh_sum = 0;
                    unsigned csum = click_in_cksum((unsigned char *) udp, len);
                    udp->uh_sum = click_in_cksum_pseudohdr(csum, ip, len);
                    output(fe->port).push(payload);
                }
                counter++;
            }
        } else {
            /*all bits were 1 - probably from a link_broadcast strategy--do not forward*/
        }
        if (pushLocally) {
            if (gc->use_mac) {
                if(in_port == 1)
                {
                    p->pull(14 + FID_LEN);
                    output(0).push(p);
                }
                if(in_port == 3)
                {
                    p->pull(14 + FID_LEN);
                    output(2).push(p);
                }
                if(in_port == 5)
                {
                    p->pull(14 + FID_LEN);
                    output(3).push(p);
                }
                if(in_port == 7)
                {
                    p->pull(14 + FID_LEN);
                    output(4).push(p);
                }
                if(in_port == 9)
                {
                    p->pull(14 + FID_LEN);
                    output(5).push(p);
                }
                if(in_port == 11)
                {
                    p->pull(14 + FID_LEN);
                    payload = p->uniqueify();
                    noofhop++ ;
                    memcpy(payload->data()+14+FID_LEN+sizeof(unsigned char)+NODEID_LEN, &noofhop, sizeof(noofhop)) ;
                    memcpy(payload->data()+14+FID_LEN+sizeof(unsigned char)+NODEID_LEN+sizeof(noofhop), reverse_FID._data, FID_LEN) ;
                    output(5).push(payload);

                }

            } else {
                p->pull(20 + 8 + FID_LEN);
                output(0).push(p);
            }
        }

        if ((out_links.size() == 0) && (!pushLocally)) {
            p->kill();
        }
    }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Forwarder)
ELEMENT_PROVIDES(ForwardingEntry)

