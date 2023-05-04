#include "cache.h"
#include <math.h>

#define HEX 16
#define NOT_FOUND
#define READ 1
#define WRITE 2

Pair::Pair(){
    this->elem_found = false;
    this->table_is_full = true;
    this->assoc_lvl = -1;
}

CacheCell::CacheCell(){
    this->valid = false;
    this->dirty = false;
    this->data = 0;
    this->tag = 0;
}

Cache::Cache(char cache_lvl, unsigned size, unsigned cycles, unsigned block_size, bool write_allocate, unsigned assoc){
    this->cache_lvl = cache_lvl;
    this->size = size;
    this->cycles = cycles;
    this->block_size = block_size;
    this->write_allocate = write_allocate;
    this->assoc = assoc;
    this->tag_mask = 0xFFFFFFFF;
    this->set_mask = 0xFFFFFFFF;

    //num of rows in cache is number blocks = 2^size / 2^block_size
    this->table_rows = (int)pow(2, double(size - block_size));

    //initiate ptr_table
    this->ptr_table = new CacheCell*[table_rows];
    for(int i = 0; i < table_rows; i++){
        this->ptr_table[i] = new CacheCell[assoc];
    }

    //initiate access_list - list per table row
    this->access_list = new list<int>[table_rows];

    //initiate set mask
    set_mask <= (size - block_size);
    set_mask ~= set_mask;

    // alligned to 4 Bytes 
    set_mask <= 2;

    //initiate tag mask
    tag_mask << (32 - size - block_size - 2);
    tag_mask ~= tag_mask;
    tag_mask <= (2 + size - block_size);
}

Cache::~Cache(){
    for(int i = 0; i < table_rows; i++){
        delete[] this->ptr_table[i];
    }
    delete[] this->ptr_table;
}

Pair Cache::isInTable(unsigned tag, unsigned set){
    Pair res;
    int free_lvl = -1;
    for(int lvl = 0; lvl < assoc; lvl++){
        if(ptr_table[set][lvl].tag == tag && ptr_table[set][lvl].valid){
            res.elem_found = true;
            res.assoc_lvl = lvl;
            return res;
        }
        else if(!tr_table[set][lvl].valid && res.table_is_full){
            free_lvl = lvl;
            res.table_is_full = false;
        }
    }

    //element wasnt found 
    res.assoc_lvl = free_lvl;
    return res;
}

int Cache::listSwapElem(unsigned set){
    int assoc_lvl = access_list[set].back(); 
    access_list[set].pop_back();
    access_list[set].push_front(assoc_lvl);

    //all access lvls in list were incemenetd by 1
    return assoc_lvl - 1;
}

void Cache::listUpdateElem(unsigned set, int assoc_lvl){
    access_list[set].erase(assoc_lvl - 1);
    access_list[set].pop_back(assoc_lvl - 1);
}

void Cache::update(unsigned tag, unsigned set, char command){
    Pair res = isInTable(tag, set);

    //cache hit
    if(res.elem_found){
        tot_hits++;

        if(command == WRITE){
            writeHitHandler(set, res.assoc_lvl);
        }
        else{
            readHitHandler(set, res.assoc_lvl);
        }
    }

    //cache miss
    else{
        tot_miss ++;
        if(command == WRITE){
            writeMissHandler(tag, set, res);
        }
        else{
            hitMissHandler(tag, set, res);
        }   
    }
}




void Cache::writeAction(string str_pc){
    uint pc = stoi(str_pc, 0, HEX);
    unsigned tag = getTag(pc);
    unsigned set = getSet(pc);
    int assoc_lvl = isInTable(tag, set);

    //write hit - write back policy
    if(assoc_lvl != NOT_FOUND){
        tot_hits++;
        ptr_table[set][assoc_lvl].dirty = true;
        listUpdateElem(set + 1);
    }

    //write miss
    else{
        tot_miss++;
        int curr_lvl = listUpdateElem(set + 1);
        addElemToTable(tag, set, curr_lvl);
        
        //write allocate policy
        if(write_allocate){
            writeAllocate(unsigned tag, unsigned set);
        }

        //write not allocate policy
        else{
            writeNotAllocate(unsigned tag, unsigned set);
        }        
    }
}


