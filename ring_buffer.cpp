#include "ring_buffer.h"

#include <stdio.h>
#include <memory.h>
#include <malloc.h>

static struct Ring_Buffer {
    uint8_t* buff;
    int read_idx;
    int write_idx;
    int size;
} rb;

#define min(a,b) (a<b ? a:b)
#define max(a,b) (a>b ? a:b)

bool rb_init(Ring_Buffer* rb, int size)
{
    memset(rb, 0, sizeof(rb));
    rb->buff = (uint8_t*)malloc(size);
    rb->size = size;

    return (rb->buff != 0);
}

void rb_free(Ring_Buffer* rb)
{
    if (rb->buff) {
        free(rb->buff);
    }
    rb->buff = NULL;
    rb->size = 0;
}

// returns number of bytes used by buffer
int rb_used(Ring_Buffer* rb)
{
    int used = rb->write_idx - rb->read_idx;
    if (used < 0) {
        used += rb->size;
    }
    printf("rb audio used after: %d\n", used);
    return used;
}

// returns number of bytes available in ringbuffer
int rb_avail(Ring_Buffer* rb)
{
    return rb->size - rb_used(rb);
}

// copy src_size bytes from src_buff to ringbuff
// if can't fit src_size, fill what's available
// returns amount written
int rb_write(Ring_Buffer* rb, uint8_t* src_buff, int src_size)
{

    int avail  = rb_avail(rb);
    int to_end = rb->size - rb->write_idx;


    int write_size = min(src_size, avail);
    int write_idx  = rb->write_idx;
    int to_end_size = min(src_size, rb->size - write_idx);

    printf("rb audio write_index: %d write_size: %d src_size: %d to_end size: %d\n", write_idx, write_size, src_size, to_end_size);

    memcpy(&rb->buff[write_idx], src_buff, to_end_size);
    if (to_end_size != write_size) {
        memcpy(&rb->buff[0], &src_buff[to_end_size], write_size - to_end_size);
    }

    write_idx += write_size;
    //TODO: what is this doing again?
    //  why do this here and not after moving index?
    if (write_idx >= rb->size) {
        write_idx -= rb->size;  //???
    }
    rb->write_idx = write_idx;

    return write_size;
}

// copy dst_size bytes from ringbuff to dst_buff
// if can't fit dst_size, read what's available
// returns amount read
int rb_read(Ring_Buffer* rb, uint8_t* dst_buff, int dst_size)
{

    int used   = rb_used(rb);
    int to_end = rb->size - rb->read_idx;


    int read_size = min(dst_size, used);
    int read_idx  = rb->read_idx;
    int to_end_size = min(read_size, rb->size - rb->read_idx);

    printf("rb audio read_index: %d read_size: %d src_size: %d to_end size: %d\n", read_idx, read_size, dst_size, to_end_size);

    memcpy(dst_buff, &rb->buff[read_idx], to_end_size);
    if (to_end_size != read_size) {
        memcpy(&dst_buff[to_end_size], &rb->buff[0], read_size - to_end_size);
    }

    read_idx += read_size;
    if (read_idx >= rb->size) {
        read_idx -= rb->size;
    }
    rb->read_idx = read_idx;
    return read_size;
}



int copy_to_ring(uint8_t* src_buff, int src_len)
{
    if (!src_buff) {
        return 0;
    }

    int written = rb_write(&rb, src_buff, src_len);
    if (written != src_len) {
        printf("Ring Buffer: Unable to write all data. %d attempted, %d written.\n", src_len, written);
    }

    return written;
}


int copy_from_ring(uint8_t* dst_buff, int dst_len)
{
    if (!dst_buff) {
        return 0;
    }

    int read = rb_read(&rb, dst_buff, dst_len);
    if (read != dst_len) {
        printf("Ring Buffer: Unable to read all data. %d attempted, %d read.\n", dst_len, read);
    }

    return read;
}

int available_ring()
{
    return rb_avail(&rb);
}

int used_ring()
{
    rb_used(&rb);
}


void init_ring(int size)
{
    bool success = rb_init(&rb, size);
}

void free_ring()
{
    rb_free(&rb);
}