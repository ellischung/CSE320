/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"
#include "helper.h"

#define SF_ALIGN 16
#define SF_ROW 8

sf_block* heap_init(void* start_of_page) {

    // initialize prologue
    int free_block_size;
    sf_prologue* prologue_p = (sf_prologue*) start_of_page;
    prologue_p -> padding1 = 0;
    prologue_p -> header = 32 | PREV_BLOCK_ALLOCATED | THIS_BLOCK_ALLOCATED;
    prologue_p -> unused1 = NULL;
    prologue_p -> unused2 = NULL;
    prologue_p -> footer = (prologue_p -> header) ^ sf_magic();

    // giant free block
    free_block_size = PAGE_SZ - (prologue_p -> header) - 8;
    sf_block* gf_block = (sf_block*) (start_of_page + 32);
    gf_block -> prev_footer = (prologue_p -> header) ^ sf_magic();
    gf_block -> header = free_block_size | PREV_BLOCK_ALLOCATED | THIS_BLOCK_ALLOCATED;
    gf_block -> body.links.next = gf_block;
    gf_block -> body.links.prev = gf_block;

    // initialize epilogue
    sf_epilogue* epilogue_p = (sf_epilogue*) (start_of_page + PAGE_SZ - 8);
    epilogue_p -> header = 0;

    // free_list_heads initialization

    sf_block* fill_empty = (sf_block*) (start_of_page + 32);
    fill_empty -> body.links.next = NULL;
    fill_empty -> body.links.prev = NULL;

    append_to_free_list(fill_empty);

    // properly fill the 7th free list with the giant block
    sf_free_list_heads[7].prev_footer = gf_block -> prev_footer;
    sf_free_list_heads[7].header = gf_block -> header;
    sf_free_list_heads[7].body.links.next = gf_block;
    sf_free_list_heads[7].body.links.prev = gf_block;
    gf_block -> body.links.next = &sf_free_list_heads[7];
    gf_block -> body.links.prev = &sf_free_list_heads[7];

    return gf_block;
}

sf_block* scan_free_list(size_t block_sz, sf_block* block) {
    sf_block* current_block = block;
    size_t current_block_sz = 0;

    while (current_block != NULL) {
        // current_block is from free list blocks
        // extract the size
        current_block_sz = (size_t) current_block -> header & BLOCK_SIZE_MASK; // changed

        // check if current block is GTE parameter size
        if (current_block_sz >= block_sz) {
            return current_block;
        }

        // move on to next block
        current_block_sz = 0;
        current_block = current_block -> body.links.next;
    }

    return NULL;
}

int append_to_free_list(sf_block* appending_block) {

    // appending_block is placed in free list
    int i;
    for(i = 0; i < NUM_FREE_LISTS; i++) {
        sf_block* current_block = &sf_free_list_heads[i];

        if (current_block != NULL) {
            current_block -> body.links.prev = appending_block;
            appending_block -> body.links.next = current_block;
            appending_block -> body.links.prev = NULL;
        }
        else {
            appending_block -> body.links.next = NULL;
            appending_block -> body.links.prev = NULL;
        }

        // free_list_heads at i = block to append to the list
        sf_free_list_heads[i] = *appending_block;
    }
    return 0;
}

int remove_from_free_list(sf_block* removing_block, sf_block* sentinel) {

    // remove the block from the free list
    // make temp next/prev blocks

    sf_block* temp_next = removing_block -> body.links.next;
    sf_block* temp_prev = removing_block -> body.links.prev;

    if ((temp_next == NULL) && (temp_prev == NULL)) {
        sentinel = NULL;
    }

    if ((temp_next != NULL) && (temp_prev == NULL)) {
        temp_next -> body.links.prev = NULL;
        sentinel = temp_next;
    }

    if ((temp_next == NULL) && (temp_prev != NULL)) {
        temp_prev -> body.links.next = NULL;
    }

    if ((temp_next != NULL) && (temp_prev != NULL)) {
        temp_next -> body.links.prev = temp_prev;
        temp_prev -> body.links.next = temp_next;
    }

    // set the removed node's next/prev to NULL
    removing_block -> body.links.next = NULL;
    removing_block -> body.links.prev = NULL;

    //printf("\n");
    //printf("\n");
    //sf_show_block(removing_block);
    //printf("\n");
    //printf("\n");

    // see what happens here ^^^^^^^
    return 0;
}

void *sf_malloc(size_t size) {

    // return null if the request is 0
    if (size == 0) {
        return NULL;
    }

    // page start variables
    void* start_of_page;
    size_t padded_size = 0;
    sf_block* temp_block;

    // check if free list have null headers, use sf_mem_grow() if all are
    if (sf_free_list_heads[0].header == 0 &&
        sf_free_list_heads[1].header == 0 &&
        sf_free_list_heads[2].header == 0 &&
        sf_free_list_heads[3].header == 0 &&
        sf_free_list_heads[4].header == 0 &&
        sf_free_list_heads[5].header == 0 &&
        sf_free_list_heads[6].header == 0 &&
        sf_free_list_heads[7].header == 0 &&
        sf_free_list_heads[8].header == 0)
    {
        start_of_page = sf_mem_grow();

        // check if there was an error with sf_mem_grow() call
        if (start_of_page == (void *)(-1)) {
            sf_errno = ENOMEM;
            return NULL;
        }

        // upon no error, initialize the memory
        temp_block = heap_init(start_of_page);

        // error using initialize_memory
        if (temp_block == NULL) {
            sf_errno = ENOMEM;
            return NULL;
        }
    }

    // heap has now been initialized, start populating free list

    // pad the size to the proper alignment
    if (size < SF_ALIGN) {
        padded_size = SF_ALIGN;
    }
    else if ((size % SF_ALIGN) == 0) {
        padded_size = size;
    }
    else {
        padded_size = ((size / SF_ALIGN) + 1) * SF_ALIGN;
    }

    // block to allocate
    sf_block* alloc_block = NULL;
    size_t alloc_block_sz = padded_size + 16; // account for header + footer

    // loop through the free lists
    int i;
    for (i = 0; i < NUM_FREE_LISTS; i++) {
        sf_block scan_block = sf_free_list_heads[i];
        alloc_block = scan_free_list(alloc_block_sz, &scan_block);

        // if allocated block isn't null, we continue
        if (alloc_block != NULL) {
            break;
        }

    }

    // start allocating (right block was found)

    size_t mem_block = (alloc_block -> header) & BLOCK_SIZE_MASK;
    size_t remainder_size = mem_block - alloc_block_sz;

    // break up the large chunk of memory
    remove_from_free_list(alloc_block, &sf_free_list_heads[i]);

    //sf_show_heap();


    // if block is bigger than minimal size, break into allocated and free blocks
    if (remainder_size >= 32) {

        sf_block* new_alloc_block = alloc_block;

        // keep track of footer?
        new_alloc_block -> prev_footer = temp_block -> prev_footer;
        new_alloc_block -> header = alloc_block_sz | THIS_BLOCK_ALLOCATED;

        // add payload?
        new_alloc_block -> body.payload[0] = size;

        // FREE block
        sf_block* new_free_block = (sf_block*) ((void*) (new_alloc_block + alloc_block_sz));

        new_free_block -> prev_footer = temp_block -> prev_footer;
        new_free_block -> header = remainder_size;
        new_free_block -> body.links.next = NULL;
        new_free_block -> body.links.prev = NULL;

        append_to_free_list(new_free_block);
    }

    // if block is fine
    sf_block* fitted_block = alloc_block;
    fitted_block -> header = mem_block;

    sf_block* new_free_block = (sf_block*) fitted_block;
    new_free_block -> body.links.next = NULL;
    new_free_block -> body.links.prev = NULL;

    append_to_free_list(new_free_block);

    return temp_block;

}

void sf_free(void *pp) {
    // if null, abort
    if (pp == NULL) {
        abort();
    }

    sf_block* block_p = (sf_block*) (pp - 8);

    size_t block_size = block_p -> header & BLOCK_SIZE_MASK;

    // if allocated bit is 0, abort
    if ((block_p -> header) != (block_p -> header | THIS_BLOCK_ALLOCATED)) {
        abort();
    }

    // if header of block is before end of prologue
    // OR if footer of block is after begining of epilogue

    if ((block_p + 8) < ((sf_block*) sf_mem_start + 32) || (block_p + 32) > ((sf_block*) sf_mem_end - 8)) {
        abort();
    }

    // if block size is less than 32, abort
    if (block_size < 32) {
        abort();
    }

    // if prev_alloc bit is 0, but alloc_field of prev is not 0
    if (((block_p -> header) & PREV_BLOCK_ALLOCATED) == (block_p -> header)) {
        if (((block_p -> body.links.prev -> header) & THIS_BLOCK_ALLOCATED) != (block_p -> body.links.prev -> header)) {
            abort();
        }
    }

    // BITWISE XOR of footer + magic != header
    if (((block_p -> body.links.next -> prev_footer) ^ sf_magic()) != (block_p -> header)) {
        abort();
    }

    // valid pp at this point
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}
