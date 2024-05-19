#include "pico/dc_offset_filter.h"

void dc_offset_filter_init(dc_offset_filter_t* self, int32_t apply_delay_samples){
	self->sample_mean_value_sum = 0;
	self->sample_mean_value_cntr = 0;
	self->sample_mean_value = 0;
	self->apply_delay_samples = apply_delay_samples;
}

int32_t dc_offset_filter_main(dc_offset_filter_t* self, int32_t input_sample, bool update_mean_val){
	int32_t sample_mean_value_approx;

	// Update mean value
	if((update_mean_val == true) && (self->sample_mean_value_cntr >=  self->apply_delay_samples)){
		self->sample_mean_value = self->sample_mean_value_sum / self->sample_mean_value_cntr;
	}

	// Use mean value if we have enough input samples
	if(self->sample_mean_value_cntr <=  self->apply_delay_samples)
		sample_mean_value_approx = 0;
	else  
		sample_mean_value_approx = self->sample_mean_value;

	// increase counters
	self->sample_mean_value_sum = self->sample_mean_value_sum + input_sample;
	self->sample_mean_value_cntr ++;

	// return output value subtracting mean value:
	return input_sample - sample_mean_value_approx;
}