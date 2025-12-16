#pragma once
#include <stdint.h>

int copy_to_ring(uint8_t* src_buff, int src_len);
int copy_from_ring(uint8_t* dst_buff, int dst_len);
int available_ring();
int used_ring();
void init_ring(int size);
void free_ring();