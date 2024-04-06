#ifndef SAMPLE_RATE_CONVERTION__H
#define SAMPLE_RATE_CONVERTION__H

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#ifndef m_new
    #define m_new(type, num) ((type *)(malloc(sizeof(type) * (num))))
#endif //m_new

#ifndef m_new_obj
    #define m_new_obj(type) (m_new(type, 1))
#endif //m_new_obj

typedef struct _src_obj_t {
	int32_t prod_stride;
	int32_t cons_stride;

	int32_t prod_ptr;
	int32_t cons_ptr;
} src_obj_t;

int32_t get_src_strride(int32_t sample_rate);

src_obj_t* create_src(uint16_t prod_sample_rate, uint16_t cons_sample_rate);

int32_t get_next_src_replications(src_obj_t* self);

#endif //SAMPLE_RATE_CONVERTION__H