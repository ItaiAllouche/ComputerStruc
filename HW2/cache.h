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

class Res{
public:
    bool table_is_full;
    bool elem_found;
    int assoc_lvl;

    // constracotr
    Res();
};

class CacheCell{
public:
    bool valid;    
    bool dirty;
    unsigned block;

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
    unsigned block_mask;
    unsigned tag_mask;
    unsigned set_mask;
    int table_rows;
    double tot_hits;
    double tot_miss;
    int mem_access;

    // constructor
    Cache(char cache_lvl, Cache* minor_cache, unsigned size, unsigned cycles, unsigned block_size, bool write_allocate, unsigned assoc);

    // distractor
    ~Cache();

    //returns pair
    //in case element was found elem_found = true
    // in case set lvl is full table_is_full = true 
    // else, assoc_lvl = free lvl
    Res isInTable(unsigned tag, unsigned set);

    // update cache accoring to corrent pc
    // operation is read/write
    void update(unsigned block, char oparation);

    // get block from pc 
    unsigned getBlock(uint pc);

    // get tag from block 
    unsigned getTag(unsigned block);

    // get set value from block 
    unsigned getSet(unsigned block);

    // in case of cache miss and the set level is full -> need to evacoate block from cache
    // finds the lru element in list, swap to new element
    // assoc_lvl is not incemented
    // returns the assoc lvl.
    int listSwapElem(unsigned set); 

    // in case of cache hit
    // update element position in list
    // assoc_lvl is not incemented
    void listUpdateElem(unsigned set, int assoc_lvl);

    // write hit handler
    void writeHitHandler(unsigned set, int assoc_lvl);

    // write miss handler
    void writeMissHandler(unsigned set, Res res, unsigned block);

    // read hit handler
    void readHitHandler(unsigned set, int assoc_lvl);

    // read miss handler
    void readMissHandler(unsigned set, Res res, unsigned block);
};

#endif // CACHE_H_