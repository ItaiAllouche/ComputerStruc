#ifndef CACHE_H_
#define CACHE_H_

#include <stdbool.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>


using namespace std;
typedef unsigned int uint;

class CacheCell{
public:
    bool valid;    
    bool dirty;
    uint tag;
    char set;

    //unnecessary? we only check if the block is in the cache
    unsigned data;

//constructor
CacheCell();
};

class Cache{
public:
    unsigned size;
    unsigned cycles;
    unsigned block_size;
    unsigned write_allocate;
    unsigned assoc;
    CacheCell** ptr_table;

    //each set has its own list.
    //elements at front are hot, elements at back are cold  
    //notice- contains lvl+1. exmp: block at lvl1 assoc will
    //represented by the number 2 
    lit* access_list;
    unsigned tag_mask;
    unsigned set_mask;
    int table_rows;
    int tot_hits;
    int tot_miss;

    //constructor
    Cache(unsigned size, unsigned cycles, unsigned block_size, bool write_allocate, unsigned assoc);

    //distractor
    ~Cache();

    //return assoc lvl is case block was founded
    // other wise return -1
    int isInTable(unsigned tag, unsigned set);

    //method for write command
    void writeAction(string str_pc);

    //method for write allocate policy - in case of write miss
    void writeAllocate(unsigned tag, unsigned set);

    //method for write not allocate policy - in case of write miss
    void writeNotAllocate(unsigned tag, unsigned set);

    //method for read command
    void readAction(uint pc);

    // get tag value from pc 
    unsigned getTag(uint pc);

    // get set value from pc 
    unsigned getSet(uint pc);

    //finds the lru element in list, swap to new element
    // and returns the access lvl.
    int listSWapElem(unsigned tag, unsigned set); 

    //update element position in list
    void listUpdateElem(unsigned tag, unsigned set); 
};

#endif //CACHE_H_