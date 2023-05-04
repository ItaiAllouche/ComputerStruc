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

class Pair{
public:
    bool table_is_full;
    bool elem_found;
    int assoc_lvl;

    //constracotr
    Pair();
}

class CacheCell{
public:
    bool valid;    
    bool dirty;
    uint tag;

    //constructor
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

    //each set has its own list.
    //elements at front are hot, elements at back are cold  
    //notice: contains lvl+1. exmp: block at lvl=1 assoc will
    //represented by the number 2 
    list* access_list;
    unsigned tag_mask;
    unsigned set_mask;
    int table_rows;
    int tot_hits;
    int tot_miss;

    //constructor
    Cache(char cache_lvl, unsigned size, unsigned cycles, unsigned block_size, bool write_allocate, unsigned assoc);

    //distractor
    ~Cache();

    //returns pair
    //in case element was found elem_found = true
    // in case set lvl is full table_is_full = true 
    // else, assoc_lvl = free lvl
    Pair isInTable(unsigned tag, unsigned set);

    //update cache accoring to corrent command
    void update(unsigned tag, unsigned set, char command);

    //get tag value from pc 
    unsigned getTag(uint pc);

    // get set value from pc 
    unsigned getSet(uint pc);

    //in case of write miss and the set level is full
    //finds the lru element in list, swap to new element
    //returns the assoc lvl.
    int listSwapElem(unsigned tag, unsigned set); 

    //in case of write hit
    //update element position in list
    void listUpdateElem(unsigned set);

    //write hit handler
    void writeHitHandler(unsigned set, int assoc_lvl);


    // //method for write command
    // void writeAction(string str_pc);

    // //method for write allocate policy - in case of write miss
    // void writeAllocate(unsigned tag, unsigned set);

    // //method for write not allocate policy - in case of write miss
    // void writeNotAllocate(unsigned tag, unsigned set);

    // //method for read command
    // void readAction(uint pc);

};

#endif //CACHE_H_