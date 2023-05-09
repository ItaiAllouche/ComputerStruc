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
    this->block = -1;
}

Cache::Cache(char cache_lvl, Cache* minor_cache, unsigned size, unsigned cycles, unsigned block_size, bool write_allocate, unsigned log_assoc){
    this->cache_lvl = cache_lvl;
    this->minor_cache = minor_cache;
    this->size = size;
    this->cycles = cycles;
    this->block_size = block_size;
    this->write_allocate = write_allocate;
    this->assoc = pow(2, double(log_assoc));
    this->block_mask = 0xFFFFFFFF;
    this->tag_mask = 0xFFFFFFFF;
    this->set_mask = 0xFFFFFFFF;
    this->tot_hits = 0;
    this->tot_miss = 0;
    this->mem_access = 0;

    // num of rows in cache is number blocks = 2^size / 2^block_size
    this->table_rows = (int)pow(2, double(size - block_size));

    // initiate ptr_table
    this->ptr_table = new CacheCell*[table_rows];
    for(int i = 0; i < table_rows; i++){
        this->ptr_table[i] = new CacheCell[assoc];
    }

    // initiate access_list - list per table row
    this->access_list = new list<int>[table_rows];

    //initiate block mask
    block_mask <<= (32 - block_size);
    block_mask = ~block_mask;
    block_mask <<= (block_size);
    // block_mask <<= (block_size + 2);
    // // block_mask = ~block_mask;
    // // block_mask <<= (block_size+2);

    printf("cache lvl is %d block_mask is 0x%x\n",cache_lvl, block_mask);

    // initiate set mask
    set_mask <<= (size - block_size);
    set_mask = ~set_mask;

    // initiate tag mask
    tag_mask <<= (32 - size);
    tag_mask = ~tag_mask;
    tag_mask <<= (size - block_size);
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
    unsigned curr_tag;
    int lvl = (int)assoc - 1;
    bool full = true;

    for( ; lvl >= 0; lvl--){
        curr_tag = getTag(ptr_table[set][lvl].block);
        if(curr_tag == tag && ptr_table[set][lvl].valid){
            res.elem_found = true;
            res.assoc_lvl = lvl;
            return res;
        }
        else if(!ptr_table[set][lvl].valid){
            free_lvl = lvl;
            full = false;;
        }
    }

    //element wasnt found 
    res.table_is_full = full;
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

void Cache::update(unsigned block, char oparation){
    unsigned tag = getTag(block);
    unsigned set = getSet(block);
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
            writeMissHandler(set, res, block);
        }
        else{
            readMissHandler(set, res, block);
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

void Cache::writeMissHandler(unsigned set, Pair res, unsigned block){

    // in cace of cache lvl 1, get block from lvl 2
    if(cache_lvl == 1){
        minor_cache->update(block, WRITE);
    }
    // cache lvl2, in case of cache miss, get block from memory
    else{
        mem_access++;
    }

    // write allocate policy
    if(write_allocate){

        // table in not full
        // update table and access hist
        if(!res.table_is_full){
            ptr_table[set][res.assoc_lvl].block = block;
            ptr_table[set][res.assoc_lvl].dirty = false;
            ptr_table[set][res.assoc_lvl].valid = true;
            access_list[set].push_front(res.assoc_lvl + 1);   
        }

        // table is full
        // replacae lru block to new block
        // update access list and update lower lvl
        else{
            //update access list
            int curr_lvl = listSwapElem(set);
            unsigned lru_block = ptr_table[set][curr_lvl].block;

            // in case of cache lvl 2
            if(cache_lvl == 2){
                
                // in case of dirty block, update in memory
                if(ptr_table[set][curr_lvl].dirty){
                    mem_access++;
                }

                //evict lru block from lv1
                unsigned minor_set = minor_cache->getSet(lru_block);
                unsigned minor_tag = minor_cache->getTag(lru_block); 
                Pair minor_res = minor_cache->isInTable(minor_tag, minor_set);
                
                if(minor_res.elem_found){
                    int minor_lvl = minor_res.assoc_lvl;
                    minor_cache->ptr_table[minor_set][minor_lvl].block = -1;
                    minor_cache->ptr_table[minor_set][minor_lvl].dirty = false;
                    minor_cache->ptr_table[minor_set][minor_lvl].valid = false;
                }
            }

            // else{

            //     // in case of dirty block, update lv2 cache
            //     if(ptr_table[set][curr_lvl].dirty){
            //         minor_cache->update(lru_block, WRITE);
            //     }               
            // }

            // update new block in cache
            ptr_table[set][curr_lvl].block = block;
            ptr_table[set][curr_lvl].dirty = false;
            ptr_table[set][curr_lvl].valid = true;              
        }
    }

    // in write no allocate policy we dont fetch block to cache
    // only in the lower lvl
}

void Cache::readMissHandler(unsigned set, Pair res, unsigned block){

    // in case of cache lvl1 bring block from cache lvl2
    if(cache_lvl == 1){
        minor_cache->update(block, READ);
    }

    //in case of cache lvl2 bring block from memory
    else{
        mem_access++;
    }

    // table in not full
    // update table and access hist
    if(!res.table_is_full){
        ptr_table[set][res.assoc_lvl].block = block;
        ptr_table[set][res.assoc_lvl].dirty = false;
        ptr_table[set][res.assoc_lvl].valid = true;
        access_list[set].push_front(res.assoc_lvl + 1);   
    }

    // table is full
    // replacae lru block to new block
    // update access list and update lower lvl
    else{
        //update access list
        int curr_lvl = listSwapElem(set);
        unsigned lru_block = ptr_table[set][curr_lvl].block;

        if(cache_lvl == 2){
            
            // in case of dirty block, update in memory
            if(ptr_table[set][curr_lvl].dirty){
                mem_access++;
            }

            //evict lru block from lv1
            unsigned minor_set = minor_cache->getSet(lru_block);
            unsigned minor_tag = minor_cache->getTag(lru_block); 
            Pair minor_res = minor_cache->isInTable(minor_tag, minor_set);

            if(minor_res.elem_found){
                int minor_lvl = minor_res.assoc_lvl;
                minor_cache->ptr_table[minor_set][minor_lvl].block = -1;
                minor_cache->ptr_table[minor_set][minor_lvl].dirty = false;
                minor_cache->ptr_table[minor_set][minor_lvl].valid = false;
            }

            // update new block in cache
            ptr_table[set][curr_lvl].block = block;
            ptr_table[set][curr_lvl].dirty = false;
            ptr_table[set][curr_lvl].valid = true;             
        }

        // else{

        //     // // in case of dirty block, update lv2 cache
        //     // if(ptr_table[set][curr_lvl].dirty){
        //     //     minor_cache->update(lru_block, WRITE);
        //     // }               
        // }
       
    }

}

unsigned Cache::getBlock(uint pc){
    unsigned block = pc & block_mask;
    block >>= (block_size);
    return block;
}

unsigned Cache::getTag(unsigned block){
    unsigned tag = block & tag_mask;
    tag >>= (size - block_size);
    return tag;
}

unsigned Cache::getSet(unsigned block){
    unsigned set = block & set_mask;
    return set;
}



