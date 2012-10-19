/*Our Proposal
 *This is the element that manipulate cache mechanism
*/
#include "cacheunit.hh"

CLICK_DECLS
/*this function matches the full Information ID*/
bool CacheEntry::matchIID(Vector<String>& fullIDs)
{
    String IID ;
    IID = fullIDs[0].substring(fullIDs[0].length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN) ;//get the information ID
    String tempSID ;
    Vector<String> forSIDupdate ;
    Vector<String>::iterator input_iter ;
    Vector<String>::iterator SIDs_iter ;
    Vector<String>::iterator IID_iter ;
    bool ret = false ;
    bool updateSID = false ;
    for( input_iter = fullIDs.begin() ; input_iter != fullIDs.end() ; input_iter++)//for each input ID
    {
        tempSID = input_iter->substring(0, input_iter->length()-PURSUIT_ID_LEN) ;//get the Scope ID
        forSIDupdate.push_back(tempSID) ;
        for(SIDs_iter = SIDs.begin() ; SIDs_iter != SIDs.end() ; SIDs_iter++)//for each local cache Scope ID
        {
            if(!(tempSID.compare(*SIDs_iter)))
            {
                updateSID = true ;
                for(IID_iter = IIDs.begin() ; IID_iter != IIDs.end() ; IID_iter++)
                {
                    if( !(IID.compare(*IID_iter)))
                        ret = true ;
                }
            }
        }
    }
    if(updateSID == true)
        SIDs = forSIDupdate ;
    return ret ;
}

//cache unit stores chunks, subscriber only specifies file ID
bool CacheEntry::matchFileID(Vector<String>& input_fileID, String& res_chunkid)
{
    for(int i = 0 ; i < input_fileID.size() ; i++)
    {
        for(int j = 0 ; j < SIDs.size() ; j++)
        {
            if( SIDs[j].substring(0, SIDs[j].length()-PURSUIT_ID_LEN) == input_fileID[i] )
            {//check the file ID 
                res_chunkid = SIDs[j].substring(SIDs[j].length()-PURSUIT_ID_LEN, PURSUIT_ID_LEN) ;
				//return the chunk ID
                return true ;
            }
        }
    }
    return false ;
}

/*this function matches Scope ID*/
bool BaseEntry::matchSID(String SID)
{
    Vector<String>::iterator sid_iter ;
    for(sid_iter = SIDs.begin() ; sid_iter != SIDs.end() ; sid_iter++)
    {
        if(!(SID.compare(*sid_iter)))
        {
            return true ;
        }
    }
    return false ;
}
/*this function matches Scope ID*/
bool BaseEntry::matchSID(Vector<String> _SIDs)
{
    Vector<String>::iterator sid_iter ;
    bool ret = false ;
    for(sid_iter = SIDs.begin() ; sid_iter != SIDs.end() ; sid_iter++)
    {
        for(Vector<String>::iterator _sid_iter = _SIDs.begin() ; _sid_iter != _SIDs.end() ; _sid_iter++)
        if(!(_sid_iter->compare(*sid_iter)))
        {
            ret = true ;
            break ;
        }
    }
    if(ret)
        SIDs = _SIDs ;
    return ret ;
}


CacheUnit::CacheUnit(){}
CacheUnit::~CacheUnit(){click_chatter("CacheUnit: destroyed!") ;}


int CacheUnit::configure(Vector<String> &conf, ErrorHandler *errh)
{
    gc = (GlobalConf*) cp_element(conf[0], this) ;
    cp_integer(conf[1], &cache_size);
    click_chatter("CU--cache_size: %d", cache_size) ;
    return 0 ;
}
int CacheUnit::initialize(ErrorHandler *errh)
{
    current_size = 0 ;
    cache.clear() ;

    Billion = 1000000000 ;
    cache_hit = 0 ;
    cache_hit_Bill = 0 ;
    cache_replace = 0 ;
    cache_replace_Bill = 0 ;
    return 0 ;
}
void CacheUnit::cleanup(CleanupStage stage)
{
    FILE *ft ;
    if( (ft = fopen("/home/cacheunit_kc.dat", "w+")) == NULL )
        click_chatter("cacheunit fopen error");
    fprintf(ft, "cache_hit: %d\ncache_hit_Bill: %d\ncache_replace: %d\ncache_replace_Bill: %d\n",
            cache_hit, cache_hit_Bill, cache_replace, cache_replace_Bill) ;
    fprintf(ft, "total_cache_number_chunk: %d\n", cache.size()) ;
    for( int i = 0 ; i < cache.size() ; i++)
    {
        fprintf(ft, "%s\n", cache[i]->SIDs[0].quoted_hex().c_str()) ;
    }
    fclose(ft) ;
    if(stage >= CLEANUP_CONFIGURED)
    {
        for(int i = 0 ; i < cache.size() ; i++)
        {
            CacheEntry* ce = cache.at(i) ;
            delete ce ;
        }
    }
    click_chatter("CachUnit: Cleaned Up!") ;
}
void CacheUnit::push(int port, Packet *p)
{
    BABitvector FID(FID_LEN*8) ;
    unsigned char numberOfIDs ;
    unsigned char IDLength /*in fragments of PURSUIT_ID_LEN each*/;
    unsigned char prefixIDLength /*in fragments of PURSUIT_ID_LEN each*/ ;
    Vector<String> IDs;
    Vector<CacheEntry*>::iterator cache_iter ;
    int index = 0 ;
    if(port == 0)
    {
        unsigned char type ;
        type = *(p->data()) ;
        switch (type)
        {
            case CINC_REQ_DATA_CACHE:
            {
                //this is a cinc sub_req message
                bool request_cache = true ;
                numberOfIDs = *(p->data() + sizeof (type));
				//get SID
                for (int i = 0; i < (int) numberOfIDs; i++) {
                    IDLength = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index);
                    IDs.push_back(String((const char *) (p->data() + sizeof (type) + sizeof (numberOfIDs) +\
                                  sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN));
                    index = index + sizeof (IDLength) + IDLength*PURSUIT_ID_LEN;
                }
                index += sizeof (numberOfIDs) ;
                p->pull(sizeof(type)) ;
                for( cache_iter = cache.begin() ; cache_iter != cache.end() ; cache_iter++ )
                {//check every cache entry
                    if((*cache_iter)->matchSID(IDs))
                    {//if cache found
                        request_cache = false ;
                        if((*cache_iter)->current_noofiid >= (*cache_iter)->total_noofiid)
                        {
                            //if I have the content
                            cache_hit++ ;
                            if(cache_hit == Billion)
                            {
                                cache_hit = 0 ;
                                cache_hit_Bill++ ;
                            }
                            BABitvector backfid(FID_LEN*8) ;
                            memcpy(backfid._data, p->data()+index, FID_LEN) ;
                            sendbackData(*cache_iter, IDs, backfid) ;
                            (*cache_iter)->last_access_time = time(NULL) ;
							//rearrange the cache
                            CacheEntry* ce = (*cache_iter) ;
                            cache.erase(cache_iter) ;
                            cache.push_back(ce) ;
                            p->kill() ;
                            return ;
                        }

                    }
                }
				//If get here, cache hit failed, notify the subscriber
                unsigned char res_type = CINC_CACHE_HIT_FAILED ;
                WritablePacket* res_packet ;
                unsigned int packet_size = FID_LEN+sizeof(res_type)+index ;
                res_packet = Packet::make(20, NULL, packet_size, 0) ;
                memcpy(res_packet->data(), p->data()+index, FID_LEN) ;
                memcpy(res_packet->data()+FID_LEN, &res_type, sizeof(res_type)) ;
                memcpy(res_packet->data()+FID_LEN+sizeof(res_type), p->data(), index) ;
                output(1).push(res_packet) ;

                if(request_cache)
                {//this cache should be cached, but it's not in the storage right now, so ask pub push it to me again
                    for(int i = 0 ; i < IDs.size() ; i++)
                    {
                        if(cache_list.find(IDs[i]) != cache_list.end())
                        {
                            WritablePacket* req_packet ;
                            unsigned char req_type = CINC_CACHE_AGAIN ;
                            unsigned char idlen = 1 ;
                            unsigned char preidlen = IDs[i].length()/PURSUIT_ID_LEN - 1 ;
                            unsigned int req_size = sizeof(req_type)+sizeof(idlen)+sizeof(preidlen)+IDs[i].length();
                            unsigned int strategy = IMPLICIT_RENDEZVOUS ;
                            req_packet = Packet::make(20, NULL, req_size, 0) ;
                            memcpy(req_packet->data(), &req_type, sizeof(req_type)) ;
                            memcpy(req_packet->data()+sizeof(req_type), &idlen, sizeof(idlen)) ;
                            memcpy(req_packet->data()+sizeof(req_type)+sizeof(idlen), IDs[i].substring(PURSUIT_ID_LEN).c_str(), PURSUIT_ID_LEN) ;
                            memcpy(req_packet->data()+sizeof(req_type)+sizeof(idlen)+PURSUIT_ID_LEN, &preidlen, sizeof(preidlen)) ;
                            memcpy(req_packet->data()+sizeof(req_type)+sizeof(idlen)+PURSUIT_ID_LEN+sizeof(preidlen),\
                                   IDs[i].substring(0,IDs[i].length()-PURSUIT_ID_LEN).c_str(), IDs[i].length()-PURSUIT_ID_LEN) ;
                            memcpy(req_packet->data()+sizeof(req_type)+sizeof(idlen)+PURSUIT_ID_LEN+sizeof(preidlen)+IDs[i].length()-PURSUIT_ID_LEN,\
                                   &strategy, sizeof(strategy)) ;
                            BABitvector RVFID(FID_LEN*8) ;
                            RVFID = gc->defaultRV_dl ;
                            WritablePacket *p1, *p2;
                            if ((RVFID.zero()) || (RVFID == gc->iLID)) {
                                /*this should be a request to the RV element running locally*/
                                /*This node is the RV point for this request*/
                                /*interact using the API - differently than below*/
                                /*these events are going to be PUBLISHED_DATA*/
                                unsigned char typeOfAPIEvent = PUBLISHED_DATA;
                                unsigned char IDLengthOfAPIEvent = gc->nodeRVScope.length() / PURSUIT_ID_LEN;
                                /***********************************************************/
                                p1 = req_packet->push(sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length());
                                memcpy(p1->data(), &typeOfAPIEvent, sizeof (typeOfAPIEvent));
                                memcpy(p1->data() + sizeof (typeOfAPIEvent), &IDLengthOfAPIEvent, sizeof (IDLengthOfAPIEvent));
                                memcpy(p1->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent), gc->nodeRVScope.c_str(), gc->nodeRVScope.length());
                                output(2).push(p1);
                            } else {
                                /*wrap the request to a publication to /FFFFFFFF/NODE_ID  */
                                /*Format: numberOfIDs = (unsigned char) 1, numberOfFragments1 = (unsigned char) 2, ID1 = /FFFFFFFF/NODE_ID*/
                                /*this should be a request to the RV element running in some other node*/
                                //click_chatter("I will send the request to the domain RV using the FID: %s", RVFID.to_string().c_str());
                                unsigned char numberOfIDs = 1;
                                unsigned char numberOfFragments = 2;
                                /*push the "header" - see above*/
                                p1 = req_packet->push(sizeof (unsigned char) + 1 * sizeof (unsigned char) + 2 * PURSUIT_ID_LEN);
                                memcpy(p1->data(), &numberOfIDs, sizeof (unsigned char));
                                memcpy(p1->data() + sizeof (unsigned char), &numberOfFragments, sizeof (unsigned char));
                                memcpy(p1->data() + sizeof (unsigned char) + sizeof (unsigned char), gc->nodeRVScope.c_str(), gc->nodeRVScope.length());
                                p2 = p1->push(FID_LEN);
                                memcpy(p2->data(), RVFID._data, FID_LEN);
                                output(0).push(p2);
                            }
                            break ;
                        }
                    }
                }
                p->kill() ;
                break ;
            }
            case CINC_PUSH_TO_CACHE:
            {
                //this is a data packet that the publisher pushes to the router, so I should cache it
                numberOfIDs = *(p->data() + sizeof (type));
				//get the SIDs, chunk level
                for (int i = 0; i < (int) numberOfIDs; i++) {
                    IDLength = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index);
                    String tempid = String((const char *) (p->data() + sizeof (type) + sizeof (numberOfIDs) +\
                                  sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN);
                    index = index + sizeof (IDLength) + IDLength*PURSUIT_ID_LEN;
                    IDs.push_back(tempid) ;
					//since publisher pushes the cache to me, it means that the content is popular;
					//so add the ID of this content to the cache_list
                    cache_list.find_insert(StringSetItem(tempid)) ;
                    click_chatter("CU: cache data: %s", IDs[0].quoted_hex().c_str()) ;
                }
                unsigned int noofiid = 0 ;
                memcpy(&noofiid, p->data() + sizeof (type) + sizeof (numberOfIDs) + index, sizeof(noofiid)) ;

                p->pull(sizeof(type)+sizeof(numberOfIDs)+index+sizeof(noofiid)) ;
                char* data = (char*) malloc(p->length()) ;
                memcpy(data, p->data(), p->length()) ;
                storecache(IDs, data, p->length(), noofiid) ;
                p->kill() ;
                break ;
            }
            case CINC_ERASE_ENTRY:
            {
				//this message is sent from the RV indicating that the cache should not be cached, due to its unpopularity
				//but I don't erase it from the storage, I just remove it from the cache_list.
                numberOfIDs = *(p->data() + sizeof (type));
                for (int i = 0; i < (int) numberOfIDs; i++) {

                    IDLength = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index);
                    String tempid = String((const char *) (p->data() + sizeof (type) + sizeof (numberOfIDs) +\
                                  sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN);
                    cache_list.erase(tempid) ;
                    index = index + sizeof (IDLength) + IDLength*PURSUIT_ID_LEN;
                    click_chatter("CU: erase entry ID: %s", tempid.quoted_hex().c_str()) ;
                }
                p->kill() ;
                break ;
            }
            case CINC_ADD_ENTRY:
            {//this message is sent from the RV indicating that this data should be cached.
			 //I don't immediately request the publisher to send the data to me.
			 //Upon the next request for this content, I'll cache this content, see CINC_REQ_DATA
                numberOfIDs = *(p->data() + sizeof (type));
                for (int i = 0; i < (int) numberOfIDs; i++) {

                    IDLength = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index);
                    String tempid = String((const char *) (p->data() + sizeof (type) + sizeof (numberOfIDs) +\
                                  sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN);
                    cache_list.find_insert(StringSetItem(tempid)) ;
                    index = index + sizeof (IDLength) + IDLength*PURSUIT_ID_LEN;
                    click_chatter("CU: add entry ID: %s", tempid.quoted_hex().c_str()) ;
                }
                p->kill() ;
                break ;
            }
            case KC_CACHE_CHECK:
            {//this message is sent from the RV to check if the router has any chunks of the file
			 //If does, send the information to subscriber, if not, just notify subscriber
			 //After check the storage, I also check the cache_list to see if I should cache some chunks of this file
                numberOfIDs = *(p->data() + sizeof (type));
				//get the file ID
                for (int i = 0; i < (int) numberOfIDs; i++) {
                    IDLength = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index);
                    IDs.push_back(String((const char *) (p->data() + sizeof (type) + sizeof (numberOfIDs) +\
                                  sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN)) ;
                    index = index + sizeof (IDLength) + IDLength*PURSUIT_ID_LEN ;
                }
                BABitvector fid2sub(FID_LEN*8) ;
                memcpy(fid2sub._data, p->data() + sizeof (type) + sizeof (numberOfIDs) + index, FID_LEN) ;
                unsigned char noofcr = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index+FID_LEN);
                Vector<String> chunkid ;
				StringSet chunkidset ;//this variable is for latter use
				//check if I have any chunks of this file
                for( int i = 0 ; i < cache.size() ; i++ )
                {
                    String tempstr ;
					if(cache[i]->matchFileID(IDs, tempstr)){
						chunkidset.find_insert(StringSetItem(tempstr)) ;
                        chunkid.push_back(tempstr) ;
					}
                }
                unsigned char response_type = 0 ;
                if(!chunkid.empty())
                {//If I have some chunks of this file, then tell the subscriber about it
                    response_type = KC_CACHE_POSITIVE ;
                    BABitvector fid2cr(FID_LEN*8) ;
                    unsigned int noofhop = 0 ;
                    unsigned char noofchunk = chunkid.size() ;
                    WritablePacket* reply_packet ;
                    unsigned int chunk_index = 0 ;
                    unsigned int packet_size = FID_LEN+sizeof(response_type)+NODEID_LEN+sizeof(noofhop)+FID_LEN+FID_LEN+\
                                               sizeof(numberOfIDs)+index+sizeof(noofchunk)+\
                                               noofchunk*PURSUIT_ID_LEN+sizeof(noofcr) ;
                    reply_packet = Packet::make(20, NULL, packet_size, 0) ;
                    fid2cr = fid2cr | gc->iLID ;
                    memcpy(reply_packet->data(), fid2sub._data, FID_LEN) ;
                    memcpy(reply_packet->data()+FID_LEN, &response_type, sizeof(response_type)) ;
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type), gc->nodeID.c_str(), NODEID_LEN) ;
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type)+NODEID_LEN, &noofhop, sizeof(noofhop)) ;
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type)+NODEID_LEN+sizeof(noofhop), fid2cr._data, FID_LEN) ;
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type)+NODEID_LEN+sizeof(noofhop)+FID_LEN, fid2sub._data, FID_LEN) ;
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type)+NODEID_LEN+sizeof(noofhop)+FID_LEN+FID_LEN,\
                           p->data()+sizeof(type), sizeof(numberOfIDs)+index) ;
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type)+NODEID_LEN+sizeof(noofhop)+FID_LEN+FID_LEN+\
                           sizeof(numberOfIDs)+index, &noofchunk, sizeof(noofchunk)) ;

                    for(int i = 0 ; i < noofchunk ; i++)
                    {
                        memcpy(reply_packet->data()+FID_LEN+sizeof(response_type)+NODEID_LEN+sizeof(noofhop)+FID_LEN+FID_LEN+\
                           sizeof(numberOfIDs)+index+sizeof(noofchunk)+chunk_index, chunkid[i].c_str(), PURSUIT_ID_LEN) ;
                        chunk_index += PURSUIT_ID_LEN ;
                    }
                    noofcr++ ;//add the publisher
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type)+NODEID_LEN+sizeof(noofhop)+FID_LEN+FID_LEN+\
                           sizeof(numberOfIDs)+index+sizeof(noofchunk)+chunk_index, &noofcr,sizeof(noofcr)) ;
                    output(3).push(reply_packet) ;
                } else{
					//If I don't have any, just notify the subscriber
                    response_type = KC_CACHE_NEGATIVE ;
                    WritablePacket* reply_packet ;
                    reply_packet = Packet::make(20, NULL, FID_LEN+sizeof(response_type)+sizeof(noofcr)+sizeof(numberOfIDs)+index, 0) ;
                    memcpy(reply_packet->data(), fid2sub._data, FID_LEN) ;
                    memcpy(reply_packet->data()+FID_LEN, &response_type, sizeof(response_type)) ;
                    noofcr++;//add the publisher
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type), &noofcr, sizeof(noofcr)) ;
                    memcpy(reply_packet->data()+FID_LEN+sizeof(response_type)+sizeof(noofcr),\
                           p->data()+sizeof(type), sizeof(numberOfIDs)+index) ;
                    output(1).push(reply_packet) ;
                }

				//the rest is cache_list check.
				for(StringSetIter iter = cache_list.begin() ; iter != cache_list.end() ; iter++)
				{
					for(int i = 0 ; i < (int) numberOfIDs; i++)
					{
						if(iter->_strData.substring(0, iter->_strData.length()-PURSUIT_ID_LEN) == IDs[i])
						{
							String tempid = iter->_strData.substring(iter->_strData.length()-PURSUIT_ID_LEN, PURSUIT_ID_LEN) ;
							if(chunkidset.find(tempid) == chunkidset.end())
							{//I should cache this chunk, but I don't have it in my storage, so ask the publisher to send it to me
								String tempfilechunkid = IDs[i]+tempid ;
								WritablePacket* req_packet ;
								unsigned char req_type = CINC_CACHE_AGAIN ;
								unsigned char idlen = 1 ;
								unsigned char preidlen = tempfilechunkid.length()/PURSUIT_ID_LEN - 1 ;
								unsigned int req_size = sizeof(req_type)+sizeof(idlen)+sizeof(preidlen)+tempfilechunkid.length();
								unsigned int strategy = IMPLICIT_RENDEZVOUS ;
								req_packet = Packet::make(20, NULL, req_size, 0) ;
								memcpy(req_packet->data(), &req_type, sizeof(req_type)) ;
								memcpy(req_packet->data()+sizeof(req_type), &idlen, sizeof(idlen)) ;
								memcpy(req_packet->data()+sizeof(req_type)+sizeof(idlen), tempfilechunkid.substring(PURSUIT_ID_LEN).c_str(), PURSUIT_ID_LEN) ;
								memcpy(req_packet->data()+sizeof(req_type)+sizeof(idlen)+PURSUIT_ID_LEN, &preidlen, sizeof(preidlen)) ;
								memcpy(req_packet->data()+sizeof(req_type)+sizeof(idlen)+PURSUIT_ID_LEN+sizeof(preidlen),\
									tempfilechunkid.substring(0,tempfilechunkid.length()-PURSUIT_ID_LEN).c_str(), tempfilechunkid.length()-PURSUIT_ID_LEN) ;
								memcpy(req_packet->data()+sizeof(req_type)+sizeof(idlen)+PURSUIT_ID_LEN+sizeof(preidlen)+tempfilechunkid.length()-PURSUIT_ID_LEN,\
									&strategy, sizeof(strategy)) ;
								BABitvector RVFID(FID_LEN*8) ;
								RVFID = gc->defaultRV_dl ;
								WritablePacket *p1, *p2;
								if ((RVFID.zero()) || (RVFID == gc->iLID)) {
									/*this should be a request to the RV element running locally*/
									/*This node is the RV point for this request*/
									/*interact using the API - differently than below*/
									/*these events are going to be PUBLISHED_DATA*/
									unsigned char typeOfAPIEvent = PUBLISHED_DATA;
									unsigned char IDLengthOfAPIEvent = gc->nodeRVScope.length() / PURSUIT_ID_LEN;
									/***********************************************************/
									p1 = req_packet->push(sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length());
									memcpy(p1->data(), &typeOfAPIEvent, sizeof (typeOfAPIEvent));
									memcpy(p1->data() + sizeof (typeOfAPIEvent), &IDLengthOfAPIEvent, sizeof (IDLengthOfAPIEvent));
									memcpy(p1->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent), gc->nodeRVScope.c_str(), gc->nodeRVScope.length());
									output(2).push(p1);
								} else {
									/*wrap the request to a publication to /FFFFFFFF/NODE_ID  */
									/*Format: numberOfIDs = (unsigned char) 1, numberOfFragments1 = (unsigned char) 2, ID1 = /FFFFFFFF/NODE_ID*/
									/*this should be a request to the RV element running in some other node*/
									//click_chatter("I will send the request to the domain RV using the FID: %s", RVFID.to_string().c_str());
									unsigned char numberOfIDs = 1;
									unsigned char numberOfFragments = 2;
									/*push the "header" - see above*/
									p1 = req_packet->push(sizeof (unsigned char) + 1 * sizeof (unsigned char) + 2 * PURSUIT_ID_LEN);
									memcpy(p1->data(), &numberOfIDs, sizeof (unsigned char));
									memcpy(p1->data() + sizeof (unsigned char), &numberOfFragments, sizeof (unsigned char));
									memcpy(p1->data() + sizeof (unsigned char) + sizeof (unsigned char), gc->nodeRVScope.c_str(), gc->nodeRVScope.length());
									p2 = p1->push(FID_LEN);
									memcpy(p2->data(), RVFID._data, FID_LEN);
									output(0).push(p2);
								}
								break ;
							}
						}
					}
					
				}
                p->kill() ;
                break ;
            }
            case KC_REQUEST_DATA:
            {//this message is from the subscriber for retrieving the data
                numberOfIDs = *(p->data() + sizeof (type));
				//get the file ID
                for (int i = 0; i < (int) numberOfIDs; i++) {
                    IDLength = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index);
                    IDs.push_back(String((const char *) (p->data() + sizeof (type) + sizeof (numberOfIDs) +\
                                  sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN)) ;
                    index = index + sizeof (IDLength) + IDLength*PURSUIT_ID_LEN ;
                }
                unsigned char noofchunk = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index);
                Vector<String> chunkid ;
                Vector<String> flushed_chunkid ;//this variable will collect the chunk IDs that have been flushed
                unsigned int chunk_index = 0 ;
				//get the chunk ID
                for(int i = 0 ; i < (int) noofchunk ; i++)
                {
                    chunkid.push_back(String((const char*) (p->data()+sizeof(type)+sizeof(numberOfIDs)+index+\
                                      sizeof(noofchunk)+chunk_index), PURSUIT_ID_LEN)) ;
                    chunk_index += PURSUIT_ID_LEN ;
                }
                BABitvector fid2sub(FID_LEN*8) ;
                memcpy(fid2sub._data, p->data()+sizeof(type)+sizeof(numberOfIDs)+index+sizeof(noofchunk)+\
                       chunk_index, FID_LEN) ;
                for(int i = 0 ; i < noofchunk ; i++)
                {
                    bool sentback = false ;
                    Vector<String> filechunkid ;
                    for(int j = 0 ; j < numberOfIDs ; j++)
                    {
                        filechunkid.push_back(IDs[j]+chunkid[i]) ;
                    }
                    for(cache_iter = cache.begin() ; cache_iter != cache.end() ; cache_iter++)
                    {
                        if((*cache_iter)->matchSID(filechunkid))
                        {
                            if((*cache_iter)->current_noofiid >= (*cache_iter)->total_noofiid)
                            {
                                //if I have the content
                                cache_hit++ ;
                                if(cache_hit == Billion)
                                {
                                    cache_hit = 0 ;
                                    cache_hit_Bill++ ;
                                }
                                sendbackData(*cache_iter, filechunkid, fid2sub) ;
                                //the cache is requested, rearrange the cache
                                (*cache_iter)->last_access_time = time(NULL) ;
                                CacheEntry* ce = (*cache_iter) ;
                                cache.erase(cache_iter) ;
                                cache.push_back(ce) ;
                                sentback = true ;
                                break ;
                            }
                        }
                    }
                    if(!sentback)
                        flushed_chunkid.push_back(chunkid[i]) ;//this chunkid has been flushed
                }
                if(!flushed_chunkid.empty())
                {//if I get here, it means that some of the chunks have been flushed, so tell the subscriber retrieve them from the publisher
                    WritablePacket* return_packet ;
                    unsigned char return_type = KC_CACHE_HIT_FAILED ;
                    unsigned char nooffailedchunk = flushed_chunkid.size() ;
                    return_packet = Packet::make(20, NULL, FID_LEN+sizeof(return_type)+sizeof(numberOfIDs)+index+\
                                                 sizeof(nooffailedchunk)+nooffailedchunk*PURSUIT_ID_LEN, 0) ;
                    memcpy(return_packet->data(), fid2sub._data, FID_LEN) ;
                    memcpy(return_packet->data()+FID_LEN, &return_type, sizeof(return_type)) ;
                    memcpy(return_packet->data()+FID_LEN+sizeof(return_type), p->data()+sizeof(type),\
                           sizeof(numberOfIDs)+index) ;
                    memcpy(return_packet->data()+FID_LEN+sizeof(return_type)+sizeof(numberOfIDs)+index,\
                           &nooffailedchunk, sizeof(nooffailedchunk)) ;
                    for(int i = 0 ; i < flushed_chunkid.size() ; i++)
                    {
                        memcpy(return_packet->data()+FID_LEN+sizeof(return_type)+sizeof(numberOfIDs)+index+\
                           sizeof(nooffailedchunk)+i*PURSUIT_ID_LEN, flushed_chunkid[i].c_str(), PURSUIT_ID_LEN) ;
                    }
                    output(1).push(return_packet) ;
                }

                p->kill() ;
                break ;
            }
        }
    }
}
void CacheUnit::sendbackData(CacheEntry* ce, Vector<String>& IDs, BABitvector& backfid)
{//send back data
    unsigned char noofid = IDs.size() ;
    for(Vector<String>::iterator cache_iter = ce->IIDs.begin() ; cache_iter != ce->IIDs.end() ; cache_iter++)
    {
        Vector<String> fullids ;
        unsigned char idlen = 0 ;
        unsigned int total_id_length = 0 ;
        unsigned int id_index = 0 ;
        for(int i = 0 ; i < IDs.size() ; i++)
        {//make the fullid
            fullids.push_back(IDs[i] + *cache_iter) ;
            total_id_length += IDs[i].length()+cache_iter->length() ;
            click_chatter("CU: send back data: %s", fullids[0].quoted_hex().c_str()) ;
        }
        WritablePacket* packet ;
        unsigned int packetsize ;

        packetsize = FID_LEN+sizeof(noofid)+noofid*sizeof(idlen)+total_id_length+ce->_data_length[*cache_iter] ;
        packet = Packet::make(20, NULL, packetsize, 0) ;
        memcpy(packet->data(), backfid._data, FID_LEN) ;
        memcpy(packet->data()+FID_LEN, &noofid, sizeof(noofid)) ;
        for(int i = 0 ; i < fullids.size() ; i++)
        {
            idlen = (unsigned char) fullids[i].length()/PURSUIT_ID_LEN ;
            memcpy(packet->data()+FID_LEN+sizeof(noofid)+id_index, &idlen, sizeof(idlen)) ;
            memcpy(packet->data()+FID_LEN+sizeof(noofid)+id_index+sizeof(idlen), fullids[i].c_str(), fullids[i].length()) ;
            id_index += sizeof(idlen)+fullids[i].length() ;
        }
        memcpy(packet->data()+FID_LEN+sizeof(noofid)+id_index, ce->_data[*cache_iter],ce->_data_length[*cache_iter]) ;
        output(0).push(packet) ;
    }

}

void CacheUnit::storecache(Vector<String>& IDs, char* data, unsigned int datalen, unsigned int noofiid)
{//store passed data packet
    bool flag = false ;
    String IID = IDs[0].substring(IDs[0].length()-PURSUIT_ID_LEN, PURSUIT_ID_LEN) ;
    for(int i = 0 ; i < IDs.size() ; i++)
    {
        //change to SID
        IDs[i] = IDs[i].substring(0, IDs[0].length()-PURSUIT_ID_LEN) ;
    }
    bool cached = false ;
    for(int i = 0 ; i < cache.size() ; i++)
    {
        if(cache[i]->matchSID(IDs))
        {
            for(int n = 0 ; n < cache[i]->IIDs.size() ; n++)
            {
                if(IID == cache[i]->IIDs[n])
                {
                    //if I have the content, I won't cache the data; note that this case should never happen
                    //since I already rule out the identical router ID in the hash function
                    click_chatter("CU: if I get here, it means the publisher push a duplicate data to me") ;
                    free(data) ;
                    return ;
                }
            }
            cache[i]->IIDs.push_back(IID) ;
            cache[i]->_data[IID] = data ;
            cache[i]->_data_length[IID] = datalen ;
            cache[i]->total_len += datalen ;
            cache[i]->current_noofiid++ ;
            current_size += datalen ;
            flag = true ;
        }
    }
    if(!flag)
    {//this means that this is the first data of this chunk, so make a new cache_entry
        cache.push_back(new CacheEntry(IDs, IID, data, datalen, noofiid)) ;
        current_size += datalen ;
        free(data) ;
    }
    if(current_size > cache_size)
    {//if overload, remove the least recently used one
        cache_replace++ ;
        if(cache_replace == Billion)
        {
            cache_replace = 0 ;
            cache_replace_Bill++ ;
        }
        CacheEntry* ce = cache[0] ;
        current_size = current_size - cache[0]->total_len ;
        cache.erase(cache.begin()) ;
        delete ce ;
    }
}


CLICK_ENDDECLS
EXPORT_ELEMENT(CacheUnit)
ELEMENT_REQUIRES(userlevel)
ELEMENT_PROVIDES(CacheEntry)
ELEMENT_PROVIDES(BaseEntry)
