#ifndef RING_BUF__H
#define RING_BUF__H

#include <stdio.h>
#include <stdbool.h>

typedef struct _ring_buf_t {
    uint8_t *buffer;
    size_t head;
    size_t tail;
    size_t size;
} ring_buf_t;

void ringbuf_init(ring_buf_t *rbuf, uint8_t *buffer, size_t size);

bool ringbuf_push(ring_buf_t *rbuf, uint8_t data);

bool ringbuf_pop(ring_buf_t *rbuf, uint8_t *data);

bool ringbuf_is_empty(ring_buf_t *rbuf);

bool ringbuf_is_full(ring_buf_t *rbuf);

size_t ringbuf_available_data(ring_buf_t *rbuf);

size_t ringbuf_available_space(ring_buf_t *rbuf);


#endif //RING_BUF__H