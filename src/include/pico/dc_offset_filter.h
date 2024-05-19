#ifndef DC_OFFSET_FILTER__H
#define DC_OFFSET_FILTER__H

#include <stdio.h>
#include <stdbool.h>

typedef struct _dc_offset_filter_t {
	int64_t sample_mean_value_sum;
	int32_t sample_mean_value_cntr;
	int32_t sample_mean_value;
	int32_t apply_delay_samples;
} dc_offset_filter_t;

void dc_offset_filter_init(dc_offset_filter_t* self, int32_t apply_delay_samples);

int32_t dc_offset_filter_main(dc_offset_filter_t* self, int32_t input_sample, bool update_mean_val);

#endif //DC_OFFSET_FILTER__H
