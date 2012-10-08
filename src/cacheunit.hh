#ifndef CACHEUNIT_HH_INCLUDED
#define CACHEUNIT_HH_INCLUDED

#include "globalconf.hh"

#include <time.h>
#include <click/etheraddress.hh>
CLICK_DECLS

class BaseEntry
{
    public:
    /**@brief default constructor*/
    BaseEntry(){}
    /**@brief destructor
     * deallocate the _data*/
    ~BaseEntry(){}
    /**@brief
     * match the fullID with the cache
     * this method will only be called, if the information ID matches
     * if match found the new IDs will be assigned to the cache IDs, since it's more update*/

    bool matchSID(String) ;
    bool matchSID(Vector<String> _SIDs) ;
    /**@brief
     * ScopeIDs that identify the Scope, since multiple SIDs may refer to the same Scope*/
    Vector<String> SIDs ;
};

/**@brief Our proposal cache entry stores one piece of cache
 */
class CacheEntry : public BaseEntry
{
public:
    /**@brief default constructor*/
    CacheEntry(){}
    /**@brief constructor
     * initialize the buffer with the datalen bytes data starting from data*/
    CacheEntry(Vector<String> newSID, String IID, char* data, int datalen, unsigned int noofiid)
    {
        SIDs = newSID ;
        _data[IID] = (char*)malloc(datalen) ;
        _data_length[IID] = datalen ;
        memcpy(_data[IID], data, datalen) ;
        IIDs.push_back(IID) ;
        total_len = datalen ;
        last_access_time = time(NULL) ;
        current_noofiid = 1 ;
        total_noofiid = noofiid ;
    }
    /**@brief destructor
     * deallocate the _data*/
    ~CacheEntry()
    {
        HashTable<String, char*>::iterator iter ;
        for( iter = _data.begin() ; iter != _data.end() ; iter++)
        {
            if(iter->second != NULL)
                free(iter->second) ;
        }
    }
    /**@brief cleans data*/
    inline void clean()
    {
        HashTable<String, char*>::iterator iter ;
        for(iter = _data.begin() ; iter != _data.end() ; iter++)
        {
            if((*iter).second != NULL)
                free((*iter).second) ;
        }
    }
    /**@brief
     * match the fullID with the cache
     * this method will only be called, if the information ID matches
     * if match found the new IDs will be assigned to the cache IDs, since it's more update*/
    bool matchIID(Vector<String>&) ;

    bool matchFileID(Vector<String>&, String&) ;

    /**@brief
     * the information ID under this scope*/
    Vector<String> IIDs ;
    /**@brief
     * the actual data*/
    HashTable<String, char*> _data ;
    /**@brief
     * the length of each item*/
    HashTable<String, unsigned int> _data_length ;
    unsigned int total_len ;
    time_t last_access_time ;
    unsigned int total_noofiid ;
    unsigned int current_noofiid ;
};

/**@brief (cinc) A Cacheunit is responsible for caching the content that it should and responding for any request
 * for the cache
 */
class CacheUnit : public Element
{
public:
    /**
     * @brief Constructor: It does nothing, as Click suggests
     */
    CacheUnit() ;
    /**
     * @brief Destructor: It does nothing, as Click suggests
     */
    ~CacheUnit() ;
    /**
     * @brief the class name - required by Click
     */
    const char *class_name() const {return "CacheUnit";}
    /**
     * @brief the input and output port number - required by Click
     * the 0 input port is connected to Classifier's 1 output
     * the 0 output port is connected to toDevice's 0 input
     */
    const char *port_count() const {return "-/-" ;}
    /**
     * @brief a PUSH Element
     */
    const char *processing() const {return PUSH ;}
    /**
     * @brief Element configuration, Cacheunit needs LIPSIN of this blackadder node
     */
    int configure(Vector<String>&, ErrorHandler*) ;
    /**
     * @brief Cacheunit is configured at the same time as Forwarder
     */
    int configure_phase() const{return 200 ;}
    /**
     * @brief This method is called by Click when the Element is about to be initialized.
     * There is nothing that needs initialization though.
     */
    int initialize(ErrorHandler *errh) ;
    /**
     * @brief Cleanup everything
     */
    void cleanup(CleanupStage stage) ;
    /**
     * @brief The core method of Cacheunit
     * Upon receiving a probing message, the Cacheunit should check its local cache to see
       if there is any cache hit. If there is, Cacheunit should modify the probing message's content.
     */
    void push(int port, Packet *p) ;
    /**@brief cinc: once cache hit, send back data to the subscriber*/
    void sendbackData(CacheEntry* ce, Vector<String>& IDs, BABitvector& backfid) ;
    /**@brief store the cache*/
    void storecache(Vector<String>&, char*, unsigned int, unsigned int) ;
    /**
     * @brief The global configuration
     */
    GlobalConf *gc ;
    /**@brief The cache*/
    Vector<CacheEntry*> cache ;
    /**@brief The total cache size in bytes*/
    unsigned int cache_size ;
    unsigned int current_size ;
    StringSet cache_list ;//cinc: this list stores the content IDs SHOULD be cached by this router
                          //note that some content may not be cached all the time due to the limit of cache size
    /*the following variables are for statistic only*/
    unsigned int cache_hit ;
    unsigned int cache_hit_Bill ;
    unsigned int cache_replace ;
    unsigned int cache_replace_Bill ;
    unsigned int Billion ;
};
CLICK_ENDDECLS
#endif // CACHEUNIT_HH_INCLUDED
