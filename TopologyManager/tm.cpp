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

#include <signal.h>
#include <arpa/inet.h>
#include <set>

#include "tm_igraph.hpp"

using namespace std;

Blackadder *ba;
TMIgraph tm_igraph;
pthread_t event_listener;

string req_id = "FFFFFFFFFFFFFFFE";
string req_prefix_id = string();
string req_bin_id = hex_to_chararray(req_id);
string req_bin_prefix_id = hex_to_chararray(req_prefix_id);

string resp_id = string();
string resp_prefix_id = "FFFFFFFFFFFFFFFD";
string resp_bin_id = hex_to_chararray(resp_id);
string resp_bin_prefix_id = hex_to_chararray(resp_prefix_id);

void handleRequest(char *request, int request_len) {
    unsigned char request_type;
    unsigned char no_publishers;
    unsigned char no_subscribers;
    string nodeID;
    set<string> publishers;
    set<string> subscribers;
    map<string, Bitvector *> result = map<string, Bitvector *>();
    map<string, Bitvector *>::iterator map_iter;
    unsigned char response_type;
    int idx = 0;
    unsigned char strategy;
    memcpy(&request_type, request, sizeof (request_type));
    memcpy(&strategy, request + sizeof (request_type), sizeof (strategy));
    if (request_type == MATCH_PUB_SUBS) {
        /*this a request for topology formation*/
        memcpy(&no_publishers, request + sizeof (request_type) + sizeof (strategy), sizeof (no_publishers));
        cout << "Publishers: ";
        for (int i = 0; i < (int) no_publishers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + idx, PURSUIT_ID_LEN);
            cout << nodeID << " ";
            idx += PURSUIT_ID_LEN;
            publishers.insert(nodeID);
        }
        cout << endl;
        cout << "Subscribers: ";
        memcpy(&no_subscribers, request + sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + idx, sizeof (no_subscribers));
        for (int i = 0; i < (int) no_subscribers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN);
            cout << nodeID << " ";
            idx += PURSUIT_ID_LEN;
            subscribers.insert(nodeID);
        }
        cout << endl;
        tm_igraph.calculateFID(publishers, subscribers, result);
        /*notify publishers*/
        for (map_iter = result.begin(); map_iter != result.end(); map_iter++) {
            if ((*map_iter).second == NULL) {
                cout << "Publisher " << (*map_iter).first << ", FID: NULL" << endl;
                response_type = STOP_PUBLISH;
                int response_size = request_len - sizeof(strategy) - sizeof (no_publishers) - no_publishers * PURSUIT_ID_LEN - sizeof (no_subscribers) - no_subscribers * PURSUIT_ID_LEN;
                char *response = (char *) malloc(response_size);
                memcpy(response, &response_type, sizeof (response_type));
                int ids_index = sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + no_publishers * PURSUIT_ID_LEN + sizeof (no_subscribers) + no_subscribers * PURSUIT_ID_LEN;
                memcpy(response + sizeof (response_type), request + ids_index, request_len - ids_index);
                /*find the FID to the publisher*/
                string destination = (*map_iter).first;
                Bitvector *FID_to_publisher = tm_igraph.calculateFID(tm_igraph.nodeID, destination);
                string response_id = resp_bin_prefix_id + (*map_iter).first;
                ba->publish_data(PUBLISH_DATA, response_id, IMPLICIT_RENDEZVOUS, (char *) FID_to_publisher->_data, FID_LEN, response, response_size);
                delete FID_to_publisher;
                free(response);
            } else {
                cout << "Publisher " << (*map_iter).first << ", FID: " << (*map_iter).second->to_string() << endl;
                response_type = START_PUBLISH;
                int response_size = request_len - sizeof(strategy) - sizeof (no_publishers) - no_publishers * PURSUIT_ID_LEN - sizeof (no_subscribers) - no_subscribers * PURSUIT_ID_LEN + FID_LEN;
                char *response = (char *) malloc(response_size);
                memcpy(response, &response_type, sizeof (response_type));
                int ids_index = sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + no_publishers * PURSUIT_ID_LEN + sizeof (no_subscribers) + no_subscribers * PURSUIT_ID_LEN;
                memcpy(response + sizeof (response_type), request + ids_index, request_len - ids_index);
                memcpy(response + sizeof (response_type) + request_len - ids_index, (*map_iter).second->_data, FID_LEN);
                /*find the FID to the publisher*/
                string destination = (*map_iter).first;
                Bitvector *FID_to_publisher = tm_igraph.calculateFID(tm_igraph.nodeID, destination);
                string response_id = resp_bin_prefix_id + (*map_iter).first;
                ba->publish_data(PUBLISH_DATA, response_id, IMPLICIT_RENDEZVOUS, (char *) FID_to_publisher->_data, FID_LEN, response, response_size);
                delete (*map_iter).second;
                delete FID_to_publisher;
                free(response);
            }
        }
    }
    if ((request_type == SCOPE_PUBLISHED) || (request_type == SCOPE_UNPUBLISHED)) {
        /*this a request to notify subscribers about a new scope*/
        memcpy(&no_subscribers, request + sizeof (request_type) + sizeof (strategy), sizeof (no_subscribers));
        for (int i = 0; i < (int) no_subscribers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (strategy) + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN);
            Bitvector *FID_to_subscriber = tm_igraph.calculateFID(tm_igraph.nodeID, nodeID);
            int response_size = request_len - sizeof(strategy) - sizeof (no_subscribers) - no_subscribers * PURSUIT_ID_LEN + FID_LEN;
            int ids_index = sizeof (request_type) + sizeof (strategy) + sizeof (no_subscribers) + no_subscribers * PURSUIT_ID_LEN;
            char *response = (char *) malloc(response_size);
            string response_id = resp_bin_prefix_id + nodeID;
            memcpy(response, &request_type, sizeof (request_type));
            memcpy(response + sizeof (request_type), request + ids_index, request_len - ids_index);
            //cout << "PUBLISHING NOTIFICATION ABOUT NEW OR DELETED SCOPE to node " << nodeID << " using FID " << FID_to_subscriber->to_string() << endl;
            ba->publish_data(PUBLISH_DATA, response_id, IMPLICIT_RENDEZVOUS, FID_to_subscriber->_data, FID_LEN, response, response_size);
            idx += PURSUIT_ID_LEN;
            delete FID_to_subscriber;
            free(response);
        }
    }
    if(request_type == SCOPE_RVS)
    {//cinc rendezvous
        set<string> crouters ;
        unsigned char noofrouter = 0 ;

        memcpy(&no_publishers, request + sizeof (request_type), sizeof (no_publishers));
        for (int i = 0; i < (int) no_publishers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + idx, PURSUIT_ID_LEN);
            idx += PURSUIT_ID_LEN;
            publishers.insert(nodeID);
        }
        noofrouter = *(request + sizeof (request_type) + sizeof (no_publishers) + idx) ;
        for(int i = 0 ; i <(int) noofrouter ; i++)
        {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + sizeof(noofrouter) + idx,\
                            PURSUIT_ID_LEN) ;
            idx += PURSUIT_ID_LEN ;
            crouters.insert(nodeID) ;
        }

        memcpy(&no_subscribers, request + sizeof (request_type) + sizeof (no_publishers) + sizeof(noofrouter)+ idx,\
                sizeof (no_subscribers));
        for (int i = 0; i < (int) no_subscribers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + sizeof(noofrouter) +\
                            sizeof (no_subscribers) + idx, PURSUIT_ID_LEN);
            idx += PURSUIT_ID_LEN;
            subscribers.insert(nodeID);
        }

        vector<string> allsids ;
        int sid_index = 0 ;
        int to_sid_index = 0 ;

        to_sid_index = sizeof(request_type)+sizeof(no_publishers)+sizeof(noofrouter)+sizeof(no_subscribers)+idx ;
        for(set<string>::iterator iter = subscribers.begin() ; iter != subscribers.end() ; iter++)
        {
            map<string, pair<Bitvector, unsigned int> > allfids ;//client to publisher or cache router fid, including distance
            map<string, Bitvector> p2sfids ;//publisher or cache router to client fid
            string tempsub = *iter ;
            unsigned char content_type = PUB ;

            tm_igraph.get_FID2crouter(crouters, publishers, tempsub,  allfids, p2sfids) ;

            char* response ;
            int tempindex = 0 ;
            unsigned char noofcontent = noofrouter + no_publishers ;
            int response_size = sizeof(response_type)+request_len - to_sid_index + sizeof(noofcontent)+\
                                allfids.size()*(NODEID_LEN+FID_LEN+sizeof(content_type)+sizeof(unsigned int)+FID_LEN);
            response = (char*) malloc(response_size) ;
            response_type = RES_FROM_TM ;
            memcpy(response, &response_type, sizeof(response_type)) ;
            memcpy(response+sizeof(response_type), request+to_sid_index, request_len-to_sid_index) ;
            memcpy(response+sizeof(response_type)+request_len-to_sid_index, &noofcontent, sizeof(noofcontent)) ;
            for(set<string>::iterator siter = publishers.begin() ; siter != publishers.end() ; siter++)
            {
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       tempindex, siter->c_str(), NODEID_LEN) ;
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       NODEID_LEN+tempindex, allfids[*siter].first._data, FID_LEN) ;
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       NODEID_LEN+FID_LEN+tempindex, &content_type, sizeof(content_type)) ;
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       NODEID_LEN+FID_LEN+tempindex+sizeof(content_type), &(allfids[*siter].second), sizeof(unsigned int)) ;
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       NODEID_LEN+FID_LEN+tempindex+sizeof(content_type)+sizeof(unsigned int), p2sfids[*siter]._data, FID_LEN) ;
                tempindex += NODEID_LEN+FID_LEN+sizeof(content_type)+sizeof(unsigned int)+FID_LEN ;
            }
            content_type = CACHE ;
            for(set<string>::iterator siter = crouters.begin() ; siter != crouters.end() ; siter++)
            {
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       tempindex, siter->c_str(), NODEID_LEN) ;
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       NODEID_LEN+tempindex, allfids[*siter].first._data, FID_LEN) ;
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       NODEID_LEN+FID_LEN+tempindex, &content_type, sizeof(content_type)) ;
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       NODEID_LEN+FID_LEN+tempindex+sizeof(content_type), &(allfids[*siter].second), sizeof(unsigned int)) ;
                memcpy(response+sizeof(response_type)+request_len-to_sid_index+sizeof(noofcontent)+\
                       NODEID_LEN+FID_LEN+tempindex+sizeof(content_type)+sizeof(unsigned int), p2sfids[*siter]._data, FID_LEN) ;
                tempindex += NODEID_LEN+FID_LEN+sizeof(content_type)+sizeof(unsigned int)+FID_LEN ;

            }

            Bitvector *FID_to_subscriber = tm_igraph.calculateFID(tm_igraph.nodeID, tempsub);
            string response_id = resp_bin_prefix_id + tempsub;
            ba->publish_data(PUBLISH_DATA, response_id, IMPLICIT_RENDEZVOUS, (char *) FID_to_subscriber->_data,\
                             FID_LEN, response, response_size);
            delete FID_to_subscriber ;
            free(response) ;
        }
    }
    if(request_type == CINC_ASK_PUB_CACHE)
    {
        memcpy(&no_publishers, request + sizeof (request_type), sizeof (no_publishers));
        for (int i = 0; i < (int) no_publishers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + idx, PURSUIT_ID_LEN);
            idx += PURSUIT_ID_LEN;
            publishers.insert(nodeID);
        }

        set<string> routerID ;
        routerID.insert(string(request + sizeof (request_type) + sizeof (no_publishers) + idx, NODEID_LEN)) ;
        tm_igraph.calculateFID(publishers, routerID, result);
        pair<string, Bitvector *> tempresult ;
        for(map<string, Bitvector *>::iterator miter = result.begin() ; miter != result.end() ; miter++)
        {
            if(miter->second != NULL)
            {
                tempresult = *miter ;
                break ;
            }
        }
        if(tempresult.second != NULL)
        {
            char* response ;
            int response_size = sizeof(response_type)+request_len-(sizeof(request_type)+\
                                sizeof(no_publishers)+idx+NODEID_LEN)+FID_LEN ;
            response = (char*) malloc(response_size) ;
            response_type = CINC_PUSH_TO_CACHE ;
            memcpy(response, &response_type, sizeof(response_type)) ;
            memcpy(response+sizeof(response_type), request+sizeof(request_type)+sizeof(no_publishers)+idx+NODEID_LEN,\
                   request_len-(sizeof(request_type)+sizeof(no_publishers)+idx+NODEID_LEN)) ;
            memcpy(response+sizeof(response_type)+request_len-(sizeof(request_type)+\
                    sizeof(no_publishers)+idx+NODEID_LEN), tempresult.second->_data, FID_LEN) ;
            Bitvector *FID_to_subscriber = tm_igraph.calculateFID(tm_igraph.nodeID, tempresult.first);
            string response_id = resp_bin_prefix_id + tempresult.first;
            ba->publish_data(PUBLISH_DATA, response_id, IMPLICIT_RENDEZVOUS, (char *) FID_to_subscriber->_data,\
                             FID_LEN, response, response_size);
            delete tempresult.second ;
            delete FID_to_subscriber ;
            free(response) ;

        }
    }
    if(request_type == CINC_ERASE_ENTRY || request_type == CINC_ADD_ENTRY)
    {
        cout<<"receive cinc operate entry message"<<endl ;
        vector<string> cache_router ;
        unsigned char noofcr = 0 ;
        Bitvector to_cr_fid(FID_LEN*8) ;
        memcpy(&noofcr, request + sizeof (request_type), sizeof (noofcr));
        for (int i = 0; i < (int) noofcr; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (noofcr) + idx, PURSUIT_ID_LEN);
            idx += PURSUIT_ID_LEN;
            cache_router.push_back(nodeID);
            cout<<"routerid: "<<nodeID<<endl ;
        }
        tm_igraph.calculateMulticastFID(tm_igraph.nodeID, cache_router, to_cr_fid) ;
        ba->notify_node(request_type, to_cr_fid, request + sizeof (request_type) + sizeof (noofcr) + idx,\
                         request_len-(sizeof (request_type) + sizeof (noofcr) + idx)) ;
    }
    if(request_type == KC_RENDEZVOUS_TYPE)
    {//kc: send to cache router to check their caches
        cout<<"receive kc_rendezvous_type"<<endl ;
        unsigned char noofrouter = 0 ;
        vector<string> crouters ;
        Bitvector to_pub_fid(FID_LEN*8) ;
        Bitvector to_cr_fid(FID_LEN*8) ;
        memcpy(&no_publishers, request + sizeof (request_type), sizeof (no_publishers));
        for (int i = 0; i < (int) no_publishers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + idx, PURSUIT_ID_LEN);
            idx += PURSUIT_ID_LEN;
            publishers.insert(nodeID);
        }
        noofrouter = *(request + sizeof (request_type) + sizeof (no_publishers) + idx) ;
        for(int i = 0 ; i <(int) noofrouter ; i++)
        {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + sizeof(noofrouter) + idx,\
                            PURSUIT_ID_LEN) ;
            idx += PURSUIT_ID_LEN ;
            crouters.push_back(nodeID) ;
        }
        memcpy(&no_subscribers, request + sizeof (request_type) + sizeof (no_publishers) + sizeof(noofrouter)+ idx,\
                sizeof (no_subscribers));
        for (int i = 0; i < (int) no_subscribers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + sizeof(noofrouter) +\
                            sizeof (no_subscribers) + idx, PURSUIT_ID_LEN);
            idx += PURSUIT_ID_LEN;
            subscribers.insert(nodeID);
        }
        unsigned int packet_size = request_len-(sizeof(request_type)+sizeof(no_publishers)+sizeof(noofrouter)+\
                                   sizeof(no_subscribers)+idx)+FID_LEN+sizeof(noofrouter) ;

//        tm_igraph.calculateMulticastFID(tm_igraph.nodeID, publishers, to_pub_fid) ;
//        Bitvector pub2sub_fid(FID_LEN * 8) ;
//        for(set<string>::iterator piter = publishers.begin() ; piter != publishers.end() ; piter++)
//        {
//            for(set<string>::iterator siter = subscribers.begin() ; siter != subscribers.end() ; siter++)
//            {
//                Bitvector *tempfid = tm_igraph.calculateFID(*piter, *siter) ;
//                pub2sub_fid = pub2sub_fid | *tempfid ;
//                delete tempfid ;
//            }
//        }
//        char* packet2pub ;
//        packet2pub = malloc(packet_size) ;
//        memcpy(packet2pub, request+sizeof(request_type)+sizeof(no_publishers)+sizeof(noofrouter)+sizeof(no_subscribers)+idx,\
//               packet_size-FID_LEN) ;
//        memcpy(packet2pub+packet_size-FID_LEN, pub2sub_fid._data, FID_LEN) ;
//        response_type = KC_PUBLISHER_CHECK ;
//        ba->notify_node(response_type, to_pub_fid, packet2pub, packet_size) ;
//        free(packet2pub) ;


        tm_igraph.calculateMulticastFID(tm_igraph.nodeID, crouters, to_cr_fid) ;
        Bitvector cache2sub_fid(FID_LEN * 8) ;
        for(vector<string>::iterator piter = crouters.begin() ; piter != crouters.end() ; piter++)
        {
            for(set<string>::iterator siter = subscribers.begin() ; siter != subscribers.end() ; siter++)
            {
                string tpub = *piter ;
                string tsub = *siter ;
                Bitvector *tempfid = tm_igraph.calculateFID(tpub, tsub) ;
                cache2sub_fid = cache2sub_fid | *tempfid ;
                delete tempfid ;
            }
        }
        char* packet2cache ;
        packet2cache = (char*) malloc(packet_size) ;
        memcpy(packet2cache, request+sizeof(request_type)+sizeof(no_publishers)+sizeof(noofrouter)+sizeof(no_subscribers)+idx,\
               packet_size-FID_LEN-sizeof(noofrouter)) ;
        memcpy(packet2cache+packet_size-FID_LEN-sizeof(noofrouter), cache2sub_fid._data, FID_LEN) ;
        memcpy(packet2cache+packet_size-sizeof(noofrouter), &noofrouter, sizeof(noofrouter)) ;
        response_type = KC_CACHE_CHECK ;
        ba->notify_node(response_type, to_cr_fid, packet2cache, packet_size) ;
        free(packet2cache) ;
    }
    if( request_type == KC_INFORM_SUB )
    {//kc: send to subscriber, tell it about the information of the best publisher
        cout<<"receive kc_inform_sub"<<endl ;
        memcpy(&no_publishers, request + sizeof (request_type), sizeof (no_publishers));

        for (int i = 0; i < (int) no_publishers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + idx, PURSUIT_ID_LEN);

            idx += PURSUIT_ID_LEN;
            publishers.insert(nodeID);
        }

        memcpy(&no_subscribers, request + sizeof (request_type) + sizeof (no_publishers) + idx, sizeof (no_subscribers));
        if(no_subscribers != 1)
        {
            cout<<"right now doesn't support concurrent subscription"<<endl ;
            return ;
        }
        for (int i = 0; i < (int) no_subscribers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (no_publishers) + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN);

            idx += PURSUIT_ID_LEN;
            subscribers.insert(nodeID);
        }
        string tempsub = *subscribers.begin() ;

        Bitvector bestpub2sub_fid(FID_LEN*8) ;
        unsigned int best_noofhops = 0 ;
        string bestpub = *publishers.begin();
        tm_igraph.calculateFID(bestpub, tempsub, bestpub2sub_fid, best_noofhops) ;
        for( set<string>::iterator iter = publishers.begin() ; iter != publishers.end() ; iter++ )
        {//get the best publisher
            Bitvector tempfid(FID_LEN*8) ;
            unsigned int temphops = 0 ;
            string temppub = *iter ;
            tm_igraph.calculateFID(temppub, tempsub, tempfid, temphops) ;
            if(temphops < best_noofhops)
            {
                bestpub = *iter ;
                bestpub2sub_fid = tempfid ;
                best_noofhops = temphops ;
            }
        }
        char* packet ;
        unsigned int packet_size = 0 ;
        packet_size = sizeof(request_type)+NODEID_LEN+sizeof(best_noofhops)+2*FID_LEN+\
                      request_len-(sizeof(request_type)+sizeof(no_publishers)+sizeof(no_subscribers)+idx) ;
        packet = (char*) malloc(packet_size) ;
        Bitvector *sub2pub = tm_igraph.calculateFID(tempsub, bestpub) ;
        memcpy(packet, &request_type, sizeof(request_type)) ;
        memcpy(packet+sizeof(request_type), bestpub.c_str(), NODEID_LEN) ;
        memcpy(packet+sizeof(request_type)+NODEID_LEN, &best_noofhops, sizeof(best_noofhops)) ;
        memcpy(packet+sizeof(request_type)+NODEID_LEN+sizeof(best_noofhops), sub2pub->_data, FID_LEN) ;
        memcpy(packet+sizeof(request_type)+NODEID_LEN+sizeof(best_noofhops)+FID_LEN, bestpub2sub_fid._data, FID_LEN) ;
        memcpy(packet+sizeof(request_type)+NODEID_LEN+sizeof(best_noofhops)+FID_LEN+FID_LEN,\
               request+sizeof(request_type)+sizeof(no_publishers)+sizeof(no_subscribers)+idx,\
               request_len-(sizeof(request_type)+sizeof(no_publishers)+sizeof(no_subscribers)+idx) ) ;
        Bitvector* tm2sub = tm_igraph.calculateFID(tm_igraph.nodeID, tempsub) ;
        string response_id = resp_bin_prefix_id + tempsub;
        ba->publish_data(PUBLISH_DATA, response_id, IMPLICIT_RENDEZVOUS, (char *) tm2sub->_data, FID_LEN, packet, packet_size);
        delete sub2pub ;
        delete tm2sub ;
        free(packet) ;

    }
}

void *event_listener_loop(void *arg) {
    Blackadder *ba = (Blackadder *) arg;
    while (true) {
        Event ev;
        ba->getEvent(ev);
        if (ev.type == PUBLISHED_DATA) {
            //cout << "TM: received a request...processing now" << endl;
            handleRequest((char *) ev.data, ev.data_len);
        } else {
            cout << "TM: I am not expecting any other notification...FATAL" << endl;
        }
    }
}

void sigfun(int sig) {
    (void) signal(SIGINT, SIG_DFL);
    cout << "TM: disconnecting" << endl;
    ba->disconnect();
    delete ba;
    cout << "TM: exiting" << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    (void) signal(SIGINT, sigfun);
    cout << "TM: starting - process ID: " << getpid() << endl;
    if (argc != 2) {
        cout << "TM: the topology file is missing" << endl;
        exit(0);
    }
    /*read the graphML file that describes the topology*/
    if (tm_igraph.readTopology(argv[1]) < 0) {
        cout << "TM: couldn't read topology file...aborting" << endl;
        exit(0);
    }
    cout << "Blackadder Node: " << tm_igraph.nodeID << endl;
    /***************************************************/
    if (tm_igraph.mode.compare("kernel") == 0) {
        ba = Blackadder::Instance(false);
    } else {
        ba = Blackadder::Instance(true);
    }
    pthread_create(&event_listener, NULL, event_listener_loop, (void *) ba);
    ba->subscribe_scope(req_bin_id, req_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    for(map<string, pair<Bitvector, int> >::iterator mapiter = tm_igraph.area_routernum.begin() ;
        mapiter != tm_igraph.area_routernum.end() ; mapiter++)
    {
        ba->notify_node(NOTIFY_AREAINFO, mapiter->second.first, (void*) &mapiter->second.second, sizeof(int)) ;
    }
    pthread_join(event_listener, NULL);
    cout << "TM: disconnecting" << endl;
    ba->disconnect();
    delete ba;
    cout << "TM: exiting" << endl;
    return 0;
}
