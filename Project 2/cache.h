/**************************************************************/
/* CS/COE 1541
 Noah Perryman
 Patrick Lyons
 ***************************************************************/

#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

struct cache_blk_t { /* note that no actual data will be stored in the cache */
  unsigned long tag;
  char valid;
  char dirty;
};

struct Node{ /*create a node data structure for the implementation of the stack*/
    
    unsigned long tag; //tag for checking stack
    struct Node *next_node; //linked list implementation for stack
    
};

struct cache_t {
	// The cache is represented by a 2-D array of blocks. 
	// The first dimension of the 2D array is "nsets" which is the number of sets (entries)
	// The second dimension is "assoc", which is the number of blocks in each set.
  int nsets;					// number of sets
  int blocksize;				// block size
  int assoc;					// associativity
  int mem_latency;				// the miss penalty
  struct cache_blk_t **blocks;	// a pointer to the array of cache blocks
  struct Node **cache_block; //an array of Node Stacks to keep track of the least recently used cache block
};

struct cache_t * cache_create(int size, int blocksize, int assoc, int mem_latency) {
    
  int i;
  int nblocks = 1;			// number of blocks in the cache
  int nsets = 1;			// number of sets (entries) in the cache

  nblocks = (size*1024) / blocksize;
  nsets = nblocks / assoc;

  struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));
		
  C->nsets = nsets; 
  C->assoc = assoc;
  C->mem_latency = mem_latency;
  C->blocksize = blocksize;
    
  C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));
  C->cache_block = (struct Node **)calloc(nsets, sizeof(struct Node*)); //allocate space for LRU stack

  for(i = 0; i < nsets; i++) {
      
      C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
      C->cache_block[i] = (struct Node *)calloc(assoc, sizeof(struct Node));
      (C->cache_block[i])->tag = 4294967295; //set tag to an impossible number as a flag
      (C->cache_block[i])->next_node = NULL;
      
  }

  return C;
    
}

/*Removes the least recently used cache block*/
unsigned long pop(struct cache_t *cache, unsigned long index) {

    if ((cache->cache_block[index])->tag == 4294967295) { /*if the stack is empty, return*/
        
        return 4294967295;
        
    }
    
    unsigned long tag = (cache->cache_block[index])->tag;
    
    
    if ((cache->cache_block[index])->next_node == NULL) { /*if there is only one node in the stack, make it null*/
        
        (cache->cache_block[index])->tag = 4294967295;
        
    } else { /*If there is more than one node, set the current node to the next node*/

        cache->cache_block[index] = (cache->cache_block[index])->next_node;
        
    }
    
    return tag;
    
}

/*cutout the node with the given tag*/
void cut(struct cache_t *cache, unsigned long index, unsigned long tag) {
    
    if ((cache->cache_block[index])->tag == 4294967295) { /*if the stack is empty, return*/
        
        return;
        
    }
    
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    new_node = (cache->cache_block[index]);
    
    struct Node *last_node = (struct Node *)malloc(sizeof(struct Node));
    last_node = (cache->cache_block[index]);
    
    int count = 0;
    int found = 0;
    
    if ((cache->cache_block[index])->next_node == NULL) { /*if there is only one node in the stack, make it null*/
        
        if ((cache->cache_block[index])->tag == tag) {
            
            (cache->cache_block[index])->tag = 4294967295; //if the tags are equal make it null
            
        }
        
    } else { /*If there is more than one node, set the current node to the next node and the last node to the prevous node*/
        
        if (new_node->tag == tag) { /*if first node is the node we are looking for*/
            found = 1;
        }
        
        while (new_node->next_node != NULL && found == 0) { /*traverse to Node with tag*/
            new_node = new_node->next_node;
            if (count != 0) {
                last_node = last_node->next_node;
            }
            count++;
            if (new_node->tag == tag) {
                found = 1;
                break;
            }
        }
        
        if (count == 0) { /*special case if it is first node and there are at least two nodes*/
            cache->cache_block[index] = (cache->cache_block[index])->next_node;
        }else {
            last_node->next_node = new_node->next_node; //remove node with tag
        }
        
    }
    
    return;
    
}

/*adds cache block to LRU stack and returns the number of nodes in the stack*/
void push(struct cache_t *cache, unsigned long index, unsigned long tag) {

    
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    new_node = (cache->cache_block[index]);
    
    struct Node *temp_node = (struct Node *)malloc(sizeof(struct Node));
    temp_node->tag = tag;
    temp_node->next_node = NULL;
    
    if ((cache->cache_block[index])->tag == 4294967295) { /*if the stack is currently empty*/
        
        cache->cache_block[index] = temp_node;
        
    }else { /*If there are nodes in the stack*/
        
        if ((cache->cache_block[index])->next_node == NULL) { /*If there is only one node in the stack*/
            (cache->cache_block[index])->next_node = temp_node;
        }else { /*If there is more than one node in the array*/
            while (new_node->next_node != NULL) { /*traverse to last Node*/
                new_node = new_node->next_node;
            }
            new_node->next_node = temp_node; /*add last node*/
            
        }
        
    }
    
    return;
    
}

/*get top of stack*/
unsigned long peek(struct cache_t *cache, unsigned long index) {
    return ((cache->cache_block[index])->tag);
}

/*calculate the index*/
unsigned long address_bits_as_index(unsigned long address, int nsets, int blocksize){
    
    unsigned long index = address;
    int shift = log(blocksize)/log(2);
    int bit_and = log(nsets)/log(2);
    index = index>>shift;
    unsigned long total = pow(2,bit_and) - 1;
    return index&total;
}

/*calculate the tag*/
unsigned long address_bits_as_tag(unsigned long address, int nsets, int blocksize){
    
    unsigned long tag = address;
    int shift_one = log(blocksize)/log(2);
    int shift_two = log(nsets)/log(2);
    tag = tag>>shift_one;
    tag = tag>>shift_two;
    return tag;
    
}

/*get the number of nodes in a particular stack*/
int num_node(struct cache_t *cache, unsigned long index) {
    
    int num_nodes = 0;
    
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    new_node = (cache->cache_block[index]);

    if ((cache->cache_block[index])->tag == 4294967295) { /*if the stack is currently empty*/

        return num_nodes;
        
    }else { /*If there are nodes in the stack*/
        
        if ((cache->cache_block[index])->next_node == NULL) { /*If there is only one node in the stack*/
            num_nodes++;
        }else { /*If there is more than one node in the array*/
            while (new_node->next_node != NULL) { /*traverse to last Node*/
                new_node = new_node->next_node;
                num_nodes++;
            }
            num_nodes++;
            
        }
        
    }
    
    return num_nodes;
    
}

/*function that determines if there is a hit or miss. 2 is returned if there is a miss with a write back.
 1 is returned if there is a miss with no write back. 0 is returned if there is a hit*/
int hit_or_miss(struct cache_t *cache, unsigned long tag, long index, int access_type) {
    
    int i;
    for(i = 0; i < cache->assoc; i++){
        if((cache->blocks[index][i]).tag == tag && (cache->blocks[index][i]).valid == 1){ //check if there is a hit
            if(access_type == 1){ //set dirty bit if we are writing to memory that is already there, indicating that if it there is going to be a block replacement the data needs to be written back
                (cache->blocks[index][i]).dirty = 1;
            }
            cut(cache, index, tag);
            push(cache, index, tag);
            return 0; //return 0 if there is a hit
        }
    }
    
    //if not a hit, then it must be a miss
    int num_nodes = num_node(cache,index); //get the number of nodes in the current LRU stack at index

    if (num_nodes < cache->assoc) { //if there are still empty spots, take them and don't replace taken ones
        push(cache, index, tag); //add tag to stack
        num_nodes = num_node(cache,index);
        cache->blocks[index][num_nodes-1].tag = tag; //set block
        cache->blocks[index][num_nodes-1].dirty = 0;
        if (access_type == 0) {
            cache->blocks[index][num_nodes-1].dirty = 0; //if a read, no need to set the dirty bit as value in cache is not changed since it was retrieved from memory
        }else {
            cache->blocks[index][num_nodes-1].dirty = 1; //if a write, set the dirty bit as value in cache has changed after it was retrieved
        }
        cache->blocks[index][num_nodes-1].valid = 1;
        return 1; //return a miss with no write back
    }
    
    unsigned long temp_tag = peek(cache, index); //get the Least recently used tag
    num_nodes = num_node(cache,index);
    
    int replacement = 0; //the block to replace
    for(i = 0; i < num_nodes; i++){
        if((cache->blocks[index][i]).tag == temp_tag){ //if this is the least recently used block, get its index
            replacement = i;
            break;
        }
        
    }
    
    if(cache->blocks[index][replacement].valid == 1){ //if the replacement block is valid, replace it and either write back or not
        if(cache->blocks[index][replacement].dirty == 0){ //if dirty is not set, write has not occurred since that data was read and no write-back is needed before replacement
            cache->blocks[index][replacement].tag = tag;
            if (access_type == 0) {
                cache->blocks[index][replacement].dirty = 0; //if a read, no need to set the dirty bit as value in cache is not changed since it was retrieved from memory
            }else {
                cache->blocks[index][replacement].dirty = 1; //if a write, set the dirty bit as value in cache has changed after it was retrieved
            }
            cache->blocks[index][replacement].valid = 1;
            pop(cache, index); //update the LRU stack
            push(cache, index, tag);
            return 1;
        }
        else if(cache->blocks[index][replacement].dirty == 1){ //if the dirty is set, we need to write back before a replacement can be made
            cache->blocks[index][replacement].tag = tag;
            if (access_type == 0) {
                cache->blocks[index][replacement].dirty = 0; //if a read, no need to set the dirty bit as value in cache is not changed since it was retrieved from memory
            }else {
                cache->blocks[index][replacement].dirty = 1; //if a write, set the dirty bit as value in cache has changed after it was retrieved
            }
            cache->blocks[index][replacement].valid = 1;
            pop(cache, index); //update the LRU stack
            push(cache, index, tag);
            return 2;
        }
    }
    return 0; //fall-through case
}

int cache_access(struct cache_t *cp, unsigned long address, int access_type)
{
  //
  // Based on "address", determine the set to access in cp and examine the blocks
  // in the set to check hit/miss and update the global hit/miss statistics
  // If a miss, determine the victim in the set to replace (LRU). 
  //
  // The function should return the hit_latency, which is 0, in case of a hit.
  // In case of a miss, the function should return mem_latency if no write back is needed.
  // If a write back is needed, the function should return 2*mem_latency.
  // access_type (0 for a read and 1 for a write) should be used to set/update the dirty bit.
  // The LRU field of the blocks in the set accessed should also be updated.
    
    int cTime = 0; //get the latency time, assume hit at first
    unsigned long index = address_bits_as_index(address,cp->nsets,cp->blocksize); //get index and tag fields
    unsigned long tag = address_bits_as_tag(address,cp->nsets,cp->blocksize);
    int the_situation = hit_or_miss(cp, tag,index, access_type); //determine if hit or miss
    //printf("\n%d\n",the_situation);
    if (the_situation == 2) { //if hit with write back, stall is 2*miss_latency
        cTime = 2*(cp->mem_latency);
    }else if (the_situation == 1){
        cTime = cp->mem_latency; //if hit with no write back, stall is miss_latency
    }
    
    return cTime;
}

#endif
