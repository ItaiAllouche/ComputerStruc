#ifndef CACHE_H_
#define CACHE_H_

#include <stdbool.h>
#include <fstream>
#include <sstream>
#include <string>
#include <list>

#define READ 1
#define WRITE 2
#define EQUAL 0
#define HEX 16


using namespace std;
typedef unsigned int uint;

class Pair{
public:
    bool table_is_full;
    bool elem_found;
    int assoc_lvl;

    // constracotr
    Pair();
};

class CacheCell{
public:
    bool valid;    
    bool dirty;
    uint tag;

    // constructor
    CacheCell();
};

class Cache{
public:
    char cache_lvl;
    Cache* minor_cache;
    unsigned size;
    unsigned cycles;
    unsigned block_size;
    unsigned write_allocate;
    unsigned assoc;
    CacheCell** ptr_table;

    // each set has its own list.
    // elements at front are hot, elements at back are cold  
    // notice: contains lvl+1. exmp: block at lvl=1 assoc will
    //r epresented by the number 2 
    list<int>* access_list;
    unsigned tag_mask;
    unsigned set_mask;
    int table_rows;
    double tot_hits;
    double tot_miss;
    int mem_access;
    double cache_access;

    // constructor
    Cache(char cache_lvl, Cache* minor_cache, unsigned size, unsigned cycles, unsigned block_size, bool write_allocate, unsigned assoc);

    // distractor
    ~Cache();

    //returns pair
    //in case element was found elem_found = true
    // in case set lvl is full table_is_full = true 
    // else, assoc_lvl = free lvl
    Pair isInTable(unsigned tag, unsigned set);

    // update cache accoring to corrent command
    void update(unsigned tag, unsigned set, char oparation);

    // get tag value from pc 
    unsigned getTag(uint pc);

    // get set value from pc 
    unsigned getSet(uint pc);

    // in case of cache miss and the set level is full
    // finds the lru element in list, swap to new element
    // returns the assoc lvl.
    int listSwapElem(unsigned set); 

    // in case of cache hit
    // update element position in list
    void listUpdateElem(unsigned set, int assoc_lvl);

    // write hit handler
    void writeHitHandler(unsigned set, int assoc_lvl);

    // write miss handler
    void writeMissHandler(unsigned tag, unsigned set, Pair res);

    // read hit handler
    void readHitHandler(unsigned set, int assoc_lvl);

    // read miss handler
    void readMissHandler(unsigned tag, unsigned set, Pair res);
};

#endif // CACHE_H_