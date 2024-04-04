/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef _PICO_PDM_MICROPHONE_H_
#define _PICO_PDM_MICROPHONE_H_

#include "hardware/pio.h"

#include "OpenPDMFilter.h"

#define MAX_PDM_RP2 (2)

#ifndef STATIC
    #define STATIC static
#endif //STATIC

#ifndef m_new
    #define m_new(type, num) ((type *)(malloc(sizeof(type) * (num))))
#endif //m_new

#ifndef m_new_obj
    #define m_new_obj(type) (m_new(type, 1))
#endif //m_new_obj

#define PDM_DECIMATION       64
#define PDM_RAW_BUFFER_COUNT 2

typedef void (*pdm_samples_ready_handler_t)(uint8_t pdm_id);

typedef struct  __pdm_microphone_config{
    uint8_t pdm_id;
    PIO pio;
    uint dma_irq;
    uint gpio_data;
    uint gpio_clk;
    uint sample_rate;
    uint sample_buffer_size;
} pdm_microphone_config;

typedef struct __pdm_mic_obj{
    uint8_t pdm_id;
    pdm_microphone_config* config;
    uint pio_sm;
    uint pio_sm_offset;
    int dma_channel;
    uint8_t* raw_buffer[PDM_RAW_BUFFER_COUNT];
    volatile int raw_buffer_write_index;
    volatile int raw_buffer_read_index;
    uint raw_buffer_size;
    TPDMFilter_InitStruct filter;
    uint16_t filter_volume;
    pdm_samples_ready_handler_t samples_ready_handler;
} pdm_mic_obj;

pdm_mic_obj* pdm_microphone_init(pdm_microphone_config* config);
void pdm_microphone_deinit(pdm_mic_obj *pdm_mic);

int pdm_microphone_start(pdm_mic_obj *pdm_mic);
void pdm_microphone_stop(pdm_mic_obj *pdm_mic);

void pdm_microphone_set_samples_ready_handler(pdm_mic_obj *pdm_mic, pdm_samples_ready_handler_t handler);
void pdm_microphone_set_filter_max_volume(pdm_mic_obj *pdm_mic, uint8_t max_volume);
void pdm_microphone_set_filter_gain(pdm_mic_obj *pdm_mic, uint8_t gain);
void pdm_microphone_set_filter_volume(pdm_mic_obj *pdm_mic, uint16_t volume);

int pdm_microphone_read(pdm_mic_obj *pdm_mic, int16_t* buffer, size_t samples);

#endif
