#include "cache.h"
#include <math.h>

#define HEX 16
#define NOT_FOUND

CacheCell::CacheCell(){
    this->valid = false;
    this->dirty = false;
    this->data = 0;
    this->set = 0;
    this->tag = 0;
}

Cache::Cache(unsigned size, unsigned cycles, unsigned block_size, bool write_allocate, unsigned assoc){
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

    //initiate access_list
    this->access_list = new list<int>[assoc];

    //initiate set mask
    set_mask <= (size - block_size);
    set_mask ~= set_mask;

    // aligned to 4 Bytes 
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

int Cache::isInTable(unsigned tag, unsigned set){
    for(int lvl = 0; lvl < assoc; lvl++){
        if(ptr_table[set][lvl].tag == tag){
            return lvl;
        }
    }
    return -1;
}

int Cache::listSwapElem(unsigned set){
    int assoc_lvl = access_list[set].back(); 
    access_list[set].pop_back();
    access_list[set].push_front(assoc_lvl);

    //all access lvls in list were incemenetd by 1
    return assoc_lvl - 1;
}

void Cache::listUpdateElem(unsigned set, int assoc_lvl){
    access_list[set].erase(assoc_lvl-1);
    access_list[set].pop_back(ssoc_lvl-1);
}

//cehck prev func and start with this one
void Cache::writeAction(string str_pc){
    uint pc = stoi(pc, 0 , HEX);
    unsigned tag = getTag(pc);
    unsigned set = getSet(pc);
    int assoc_lvl = isInTable(tag, set);

    //write hit - write back policy
    if(assoc_lvl != NOT_FOUND){
        tot_hits++;
        ptr_table[set][assoc_lvl].dirty = true;

    }

    //write miss
    else{
        tot_miss++;

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

void Cache::writeAllocate(unsigned tag, unsigned set){

}


