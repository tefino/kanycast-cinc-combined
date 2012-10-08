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

#ifndef BLACKADDER_DEFS_HPP
#define BLACKADDER_DEFS_HPP

/**********************************/
#define PURSUIT_ID_LEN 8 //in bytes
#define FID_LEN 32 //in bytes
#define NODEID_LEN PURSUIT_ID_LEN //in bytes
#define AREA_LENGTH 3 //cinc: the length of area ID in bytes
#define HIGHTLEVEL 3//cinc: the hierachy level
#define EACHAREA 1 //cinc: the length of each area ID
#define LASTLENGTH 5 //cinc: the length of the last component of node ID
/****some strategies*****/
#define NODE_LOCAL          0
#define LINK_LOCAL          1
#define DOMAIN_LOCAL        2
#define IMPLICIT_RENDEZVOUS 3
#define BROADCAST_IF        4
/************************/
/*intra and inter click message types*/
#define PUBLISH_SCOPE 0
#define PUBLISH_INFO 1
#define UNPUBLISH_SCOPE 2
#define UNPUBLISH_INFO 3
#define SUBSCRIBE_SCOPE 4
#define SUBSCRIBE_INFO 5
#define UNSUBSCRIBE_SCOPE 6
#define UNSUBSCRIBE_INFO 7
#define PUBLISH_DATA  8 //the request



#define CONNECT 12
#define DISCONNECT 13
/*****************************/
#define UNDEF_EVENT 0
#define START_PUBLISH 100
#define STOP_PUBLISH 101
#define SCOPE_PUBLISHED 102
#define SCOPE_UNPUBLISHED 103
#define PUBLISHED_DATA 104
#define MATCH_PUB_SUBS 105
#define RV_RESPONSE 106
//our proposal this is for subinfo message destined at publisher
#define PLEASE_PUSH_DATA 107
//cinc information item published
#define INFO_PUBLISHED 108
//cinc tm match pub sub under scope

#define SCOPE_RVS 109
//cinc pub send scope probing message
#define SUB_REQ 110
//cinc sub request to cache routers
#define NOTIFY_AREAINFO 111
//cinc: notify routers about the # of routers in their areas
#define RES_DATA 112
//cinc: response with data
#define CINC_SUB_SCOPE 113
//cinc: subscrip scope
#define RES_FROM_TM 114
//cinc: response from TM including all the FID information
#define CINC_REQ_DATA_CACHE 115
//cinc: request for data from a cache router
#define CINC_REQ_DATA_PUB 116
//cinc: request for data from a publisher node
#define CINC_CACHE_HIT_FAILED 117
//cinc: response from router cache hit failed
#define CINC_ASK_PUB_CACHE 118
/*cinc: RV ask TM to calculate the path from pub to cache router
 *notify the pub to push data to cache router for caching*/
#define CINC_PUSH_TO_CACHE 119
//cinc: RV/TM notify publisher to push data to router for caching
#define CINC_ERASE_ENTRY 120
//cinc: RV notify router erase a cache list entry
#define CINC_CACHE_AGAIN 121
//cinc: cache router ask pub push cache again, this event happens when the cache is flushed in the cache router,
//but its popularity is still high and is requested again by a client
#define CINC_ADD_ENTRY 122
//cinc: RV notify router add a cache list entry

#define KC_PUBLISH_SCOPE 123
//kc: kanycast cinc combination, specify a scope_type, e.g. file, chunk or segment
#define KC_SUB_SCOPE 124
//kc: subscribe scope
#define KC_RENDEZVOUS_TYPE 125
//kc: rv find the publisher and cache router, tm calculate the path
#define KC_PUBLISHER_CHECK 126
//kc: rv ask publisher to check their state and report to subscriber
#define KC_CACHE_CHECK 127
//kc: rv ask cache router to chech their state and report to subscriber
#define KC_INFORM_SUB 128
//kc: rv collect the chunks under a file and inform subscriber with the assistance of TM
#define KC_CACHE_POSITIVE 129
//kc: cache router finds the content and tell the subscriber about it
#define KC_CACHE_NEGATIVE 130
//kc: cache router doesn't find the content and tell teh subscriber
#define KC_REQUEST_DATA 131
//kc: ask cache router or publisher to push data


/*the type of scope*/
#define UNSPECIFY 0
#define FILE_LEVEL 1
#define CHUNK_LEVEL 2




#define PUB 1
#define CACHE 2

#define NETLINK_BADDER 20

/*****************************/
#define SCOPE_PROBING_MESSAGE 1
#define SUB_SCOPE_MESSAGE 2
#endif /* BLACKADDER_DEFS_HPP */
