#include "sample_rate_convertion.h"

int32_t get_src_strride(int32_t sample_rate){
	return 7056000/sample_rate;
}

src_obj_t* create_src(uint16_t prod_sample_rate, uint16_t cons_sample_rate){

	src_obj_t *self = m_new_obj(src_obj_t);
	memset(self, 0, sizeof(src_obj_t));

	self->prod_stride = get_src_strride(prod_sample_rate);
	self->cons_stride = get_src_strride(cons_sample_rate);

	self->prod_ptr = 0;
	self->cons_ptr = 0;

	return self;
}

int32_t get_next_src_replications(src_obj_t* self){
	if(self->prod_stride == self->cons_stride)
		return 1;
	else{
		int32_t res = 0;

		self->prod_ptr += self->prod_stride;

		while(self->prod_ptr > self->cons_ptr) {
			res ++;
			self->cons_ptr += self->cons_stride;
		}

		if(self->prod_ptr == self->cons_ptr){
			self->prod_ptr = 0;
			self->cons_ptr = 0;
		}
		else{
			if(self->prod_ptr > self->cons_ptr){
				self->prod_ptr = self->prod_ptr - self->cons_ptr;
				self->cons_ptr = 0;
			}
			else {
				self->cons_ptr = self->cons_ptr - self->prod_ptr;
				self->prod_ptr = 0;
			}
		}

		return res;
	}
}
