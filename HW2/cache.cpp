#include "cache.h"
#include <math.h>

Pair::Pair(){
    this->elem_found = false;
    this->table_is_full = true;
    this->assoc_lvl = -1;
}

CacheCell::CacheCell(){
    this->valid = false;
    this->dirty = false;
    this->tag = 0;
}

Cache::Cache(char cache_lvl, Cache* minor_cache, unsigned size, unsigned cycles, unsigned block_size, bool write_allocate, unsigned assoc){
    this->cache_lvl = cache_lvl;
    this->minor_cache = minor_cache;
    this->size = size;
    this->cycles = cycles;
    this->block_size = block_size;
    this->write_allocate = write_allocate;
    this->assoc = assoc;
    this->tag_mask = 0xFFFFFFFF;
    this->set_mask = 0xFFFFFFFF;
    this->tot_hits = 0;
    this->tot_miss = 0;
    this->mem_access = 0;
    this-> cache_access = 0;

    // num of rows in cache is number blocks = 2^size / 2^block_size
    this->table_rows = (int)pow(2, double(size - block_size));

    // initiate ptr_table
    this->ptr_table = new CacheCell*[table_rows];
    for(int i = 0; i < table_rows; i++){
        this->ptr_table[i] = new CacheCell[assoc + 1];
    }

    // initiate access_list - list per table row
    this->access_list = new list<int>[table_rows];

    // initiate set mask
    set_mask <<= (size - block_size);
    set_mask = ~set_mask;

    // alligned to 4 Bytes 
    set_mask <<= 2;

    // initiate tag mask
    tag_mask <<= (32 - size - block_size - 2);
    tag_mask = ~tag_mask;
    tag_mask <<= (2 + size - block_size);
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
    for(int lvl = 0; lvl <= (int)assoc; lvl++){
        if(ptr_table[set][lvl].tag == tag && ptr_table[set][lvl].valid){
            res.elem_found = true;
            res.assoc_lvl = lvl;
            return res;
        }
        else if(!ptr_table[set][lvl].valid && res.table_is_full){
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
    list<int>::iterator it;
    for(it = access_list[set].begin(); it != access_list[set].end(); it++){
        if(*it == assoc_lvl + 1){
            break;
        }
    }
    access_list[set].erase(it);
    access_list[set].push_front(assoc_lvl + 1);
}

void Cache::update(unsigned tag, unsigned set, char oparation){
    cache_access++;
    Pair res = isInTable(tag, set);

    //cache hit
    if(res.elem_found){
        tot_hits++;

        if(oparation == WRITE){
            writeHitHandler(set, res.assoc_lvl);
        }
        else{
            readHitHandler(set, res.assoc_lvl);
        }
    }

    //cache miss
    else{
        tot_miss++;
        if(oparation == WRITE){
            writeMissHandler(tag, set, res);
        }
        else{
            readMissHandler(tag, set, res);
        }   
    }
}

void Cache::writeHitHandler(unsigned set, int assoc_lvl){
    //write back policy
    ptr_table[set][assoc_lvl].dirty = true;

    //update access_list
    listUpdateElem(set, assoc_lvl);
}

void Cache::readHitHandler(unsigned set, int assoc_lvl){
    listUpdateElem(set, assoc_lvl);
}

void Cache::writeMissHandler(unsigned tag, unsigned set, Pair res){

    // in cace of cache lvl 1 search block in lvl 2
    if(cache_lvl == 1){
        minor_cache->update(tag, set, WRITE);
    }
    // cache lvl2, in case of cache miss, look for element in memory
    else{
        mem_access++;
    }

    // write allocate policy
    if(write_allocate){

        // table in not full
        // update table and access hist
        if(!res.table_is_full){
            ptr_table[set][res.assoc_lvl].tag = tag;
            ptr_table[set][res.assoc_lvl].dirty = false;
            ptr_table[set][res.assoc_lvl].valid = true;
            access_list[set].push_front(res.assoc_lvl + 1);   
        }

        // table is full
        // replacae lru block to new block
        // update access list and update lower lvl
        else{
            int curr_lvl = listSwapElem(set);
            unsigned lru_tag = ptr_table[set][curr_lvl].tag;

            // in case of cache lvl 2
            if(cache_lvl == 2){
                
                // in case of dirty block, update in memory
                if(ptr_table[set][curr_lvl].dirty){
                    mem_access++;
                }
            }

            else{

                // in case of dirty block, update lv2 cache
                if(ptr_table[set][curr_lvl].dirty){
                    minor_cache->update(lru_tag, set, WRITE);
                }               
            }

            // update new block in cache
            ptr_table[set][curr_lvl].tag = tag;
            ptr_table[set][curr_lvl].dirty = false;              
        }
    }

    // in write no allocate policy we dont fetch block to cache
    // only in the lower lvl
}

void Cache::readMissHandler(unsigned tag, unsigned set, Pair res){

    // in case of cache lvl1 bring block from cache lvl2
    if(cache_lvl == 1){
        minor_cache->update(tag, set, READ);
    }

    //in case of cache lvl2 bring block from memory
    else{
        mem_access++;
    }

    // table in not full
    // update table and access hist
    if(!res.table_is_full){
        ptr_table[set][res.assoc_lvl].tag = tag;
        ptr_table[set][res.assoc_lvl].dirty = false;
        ptr_table[set][res.assoc_lvl].valid = true;
        access_list[set].push_front(res.assoc_lvl + 1);   
    }

    // table is full
    // replacae lru block to new block
    // update access list and update lower lvl
    else{
        int curr_lvl = listSwapElem(set);
        unsigned lru_tag = ptr_table[set][curr_lvl].tag;

        if(cache_lvl == 2){
            
            // in case of dirty block, update in memory
            if(ptr_table[set][curr_lvl].dirty){
                mem_access++;
            }
        }

        else{

            // in case of dirty block, update lv2 cache
            if(ptr_table[set][curr_lvl].dirty){
                minor_cache->update(lru_tag, set, WRITE);
            }               
        }

        // update new block in cache and update access list
        ptr_table[set][curr_lvl].tag = tag;
        ptr_table[set][curr_lvl].dirty = false;           
    }

}

unsigned Cache::getTag(uint pc){
    unsigned tag = pc & tag_mask;
    tag >>= (2 + size - block_size);
    return tag;
}

unsigned Cache::getSet(uint pc){
    unsigned set = pc & set_mask;
    set >>= 2;
    return set;
}



