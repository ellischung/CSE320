#ifndef HELPER_H
#define HELPER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "sfmm.h"

sf_block* heap_init(void* start_of_page);

sf_block* scan_free_list(size_t block_sz, sf_block* block);

int append_to_free_list(sf_block* appending_block);

int remove_from_free_list(sf_block* removing_block, sf_block* sentinel);








#endif