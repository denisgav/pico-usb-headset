/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "pdm_microphone.pio.h"

#include "pico/pdm_microphone.h"

STATIC pdm_mic_obj* pdm_mic_inst[MAX_PDM_RP2] = {NULL, NULL};

STATIC const PIO pio_instances[NUM_PIOS] = {pio0, pio1};


void pdm_dma_handler(uint8_t pdm_id);
STATIC void pdm_dma_irq0_handler();
STATIC void pdm_dma_irq1_handler();

pdm_mic_obj* pdm_microphone_init(pdm_microphone_config* config) {
    uint8_t pdm_id = config->pdm_id;
    if(pdm_id >= MAX_PDM_RP2)
        return NULL;

    if (config->sample_buffer_size % (config->sample_rate / 1000)) {
        return NULL;
    }

    pdm_mic_obj* pdm_mic;
    if(pdm_mic_inst[pdm_id] == NULL){
        pdm_mic = m_new_obj(pdm_mic_obj);

        pdm_mic_inst[pdm_id] = pdm_mic;
    } else {
        pdm_mic = pdm_mic_inst[pdm_id];

        pdm_microphone_deinit(pdm_mic);
    }


    memset(pdm_mic, 0x00, sizeof(pdm_mic));
    pdm_mic->config = config;
    pdm_mic->pdm_id = pdm_id;

    // if(pdm_mic->pdm_id == 0)
    // {
    //     pdm_mic->config->dma_irq = DMA_IRQ_0;
    // }
    // else
    // {
    //     pdm_mic->config->dma_irq = DMA_IRQ_1;
    // }
    // pdm_mic->config->dma_irq = DMA_IRQ_0;


    pdm_mic->raw_buffer_size = config->sample_buffer_size * (PDM_DECIMATION / 8);

    for (int i = 0; i < PDM_RAW_BUFFER_COUNT; i++) {
        pdm_mic->raw_buffer[i] = malloc(pdm_mic->raw_buffer_size);
        if (pdm_mic->raw_buffer[i] == NULL) {
            pdm_microphone_deinit(pdm_mic);

            return NULL;   
        }
    }

    pdm_mic->dma_channel = dma_claim_unused_channel(true);
    if (pdm_mic->dma_channel < 0) {
        pdm_microphone_deinit(pdm_mic);

        return NULL;
    }

    pdm_mic->pio_sm_offset = pio_add_program(pdm_mic->config->pio, &pdm_microphone_data_program);
    pdm_mic->pio_sm = pio_claim_unused_sm(pdm_mic->config->pio, true);
    

    float clk_div = clock_get_hz(clk_sys) / (config->sample_rate * PDM_DECIMATION * 4.0);

    pdm_microphone_data_init(
        pdm_mic->config->pio,
        pdm_mic->pio_sm,
        pdm_mic->pio_sm_offset,
        clk_div,
        config->gpio_data,
        config->gpio_clk
    );

    dma_channel_config dma_channel_cfg = dma_channel_get_default_config(pdm_mic->dma_channel);

    channel_config_set_transfer_data_size(&dma_channel_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_channel_cfg, false);
    channel_config_set_write_increment(&dma_channel_cfg, true);
    channel_config_set_dreq(&dma_channel_cfg, pio_get_dreq(pdm_mic->config->pio, pdm_mic->pio_sm, false));

    dma_channel_configure(
        pdm_mic->dma_channel,
        &dma_channel_cfg,
        pdm_mic->raw_buffer[0],
        (void *)&pdm_mic->config->pio->rxf[pdm_mic->pio_sm],
        pdm_mic->raw_buffer_size,
        false
    );

    pdm_mic->filter.Fs = config->sample_rate;
    pdm_mic->filter.LP_HZ = config->sample_rate / 2;
    pdm_mic->filter.HP_HZ = 10;
    pdm_mic->filter.In_MicChannels = 1;
    pdm_mic->filter.Out_MicChannels = 1;
    pdm_mic->filter.Decimation = PDM_DECIMATION;
    pdm_mic->filter.MaxVolume = 64;
    pdm_mic->filter.Gain = 16;

    pdm_mic->filter_volume = pdm_mic->filter.MaxVolume;

    pdm_mic->raw_buffer_write_index = 0;
    pdm_mic->raw_buffer_read_index = 0;

    return pdm_mic;
}

void pdm_microphone_deinit(pdm_mic_obj *pdm_mic) {
    for (int i = 0; i < PDM_RAW_BUFFER_COUNT; i++) {
        if (pdm_mic->raw_buffer[i]) {
            free(pdm_mic->raw_buffer[i]);

            pdm_mic->raw_buffer[i] = NULL;
        }
    }

    if (pdm_mic->dma_channel > -1) {
        dma_channel_unclaim(pdm_mic->dma_channel);

        pdm_mic->dma_channel = -1;
    }

    pdm_mic_inst[pdm_mic->pdm_id] == NULL;
}

static int irq0_handler_cntr = 0;
static int irq1_handler_cntr = 0;

int pdm_microphone_start(pdm_mic_obj *pdm_mic) {
    irq_set_enabled(pdm_mic->config->dma_irq, true);
    dma_irqn_acknowledge_channel(pdm_mic->config->dma_irq, pdm_mic->dma_channel);
    dma_irqn_set_channel_enabled(pdm_mic->config->dma_irq, pdm_mic->dma_channel, true);
    if(pdm_mic->config->dma_irq == DMA_IRQ_0)
    {
        dma_channel_set_irq0_enabled(pdm_mic->dma_channel, true);
        if(irq0_handler_cntr == 0)
        {
            irq_set_exclusive_handler(pdm_mic->config->dma_irq, pdm_dma_irq0_handler);
        }
        irq0_handler_cntr++;
    }
    else
    {
        dma_channel_set_irq1_enabled(pdm_mic->dma_channel, true);
        if(irq1_handler_cntr == 0)
        {
            irq_set_exclusive_handler(pdm_mic->config->dma_irq, pdm_dma_irq1_handler);
        }
        irq1_handler_cntr++;
    }
    
    Open_PDM_Filter_Init(&pdm_mic->filter);

    pio_sm_set_enabled(
        pdm_mic->config->pio,
        pdm_mic->pio_sm,
        true
    );

    pdm_mic->raw_buffer_write_index = 0;
    pdm_mic->raw_buffer_read_index = 0;

    //-------- Strange workaround ------------
    dma_channel_transfer_to_buffer_now(
        pdm_mic->dma_channel,
        pdm_mic->raw_buffer[0],
        pdm_mic->raw_buffer_size
    );


    //----------------------------------------

    return 0;
}

void pdm_microphone_stop(pdm_mic_obj *pdm_mic) {
    pio_sm_set_enabled(
        pdm_mic->config->pio,
        pdm_mic->pio_sm,
        false
    );

    dma_channel_abort(pdm_mic->dma_channel);

    if (pdm_mic->config->dma_irq == DMA_IRQ_0) {
        dma_channel_set_irq0_enabled(pdm_mic->dma_channel, false);
    } else if (pdm_mic->config->dma_irq == DMA_IRQ_1) {
        dma_channel_set_irq1_enabled(pdm_mic->dma_channel, false);
    }

    irq_set_enabled(pdm_mic->config->dma_irq, false);
}

STATIC uint dma_map_irq_to_channel(uint irq_index) {
    for (uint ch = 0; ch < NUM_DMA_CHANNELS; ch++) {
        if ((dma_irqn_get_channel_status(irq_index, ch))) {
            return ch;
        }
    }
    // This should never happen
    return -1;
}

STATIC void pdm_dma_handle_mic(pdm_mic_obj *pdm_mic)
{
    // get the next capture index to send the dma to start
    pdm_mic->raw_buffer_read_index = pdm_mic->raw_buffer_write_index;
    pdm_mic->raw_buffer_write_index = (pdm_mic->raw_buffer_write_index + 1) % PDM_RAW_BUFFER_COUNT;
    
    // give the channel a new buffer to write to and re-trigger it
    dma_channel_transfer_to_buffer_now(
        pdm_mic->dma_channel,
        pdm_mic->raw_buffer[pdm_mic->raw_buffer_write_index],
        pdm_mic->raw_buffer_size
    );
    // dma_channel_set_write_addr(
    //     pdm_mic->dma_channel, 
    //     pdm_mic->raw_buffer[pdm_mic->raw_buffer_write_index], 
    //     false);

    if (pdm_mic->samples_ready_handler) {
        pdm_mic->samples_ready_handler(pdm_mic->pdm_id);
    }
}

void pdm_dma_handler(uint8_t irq_index) {
    for(uint pdm_idx =0; pdm_idx < MAX_PDM_RP2; pdm_idx++) {
        if((pdm_mic_inst[pdm_idx] != NULL) && (pdm_mic_inst[pdm_idx]->config->dma_irq == irq_index)){
            uint ch = pdm_mic_inst[pdm_idx]->dma_channel;

            if ((dma_irqn_get_channel_status(irq_index, ch))) {
                pdm_mic_obj *pdm_mic = pdm_mic_inst[pdm_idx];

                pdm_dma_handle_mic(pdm_mic);

                // clear IRQ
                dma_irqn_acknowledge_channel(irq_index, ch);
            }
        }
    }
}

STATIC void pdm_dma_irq0_handler()
{
    pdm_dma_handler(DMA_IRQ_0);
}

STATIC void pdm_dma_irq1_handler()
{
    pdm_dma_handler(DMA_IRQ_1);
}

void pdm_microphone_set_samples_ready_handler(pdm_mic_obj *pdm_mic, pdm_samples_ready_handler_t handler) {
    pdm_mic->samples_ready_handler = handler;
}

void pdm_microphone_set_filter_max_volume(pdm_mic_obj *pdm_mic, uint8_t max_volume) {
    pdm_mic->filter.MaxVolume = max_volume;
}

void pdm_microphone_set_filter_gain(pdm_mic_obj *pdm_mic, uint8_t gain) {
    pdm_mic->filter.Gain = gain;
}

void pdm_microphone_set_filter_volume(pdm_mic_obj *pdm_mic, uint16_t volume) {
    pdm_mic->filter_volume = volume;
}

int pdm_microphone_read(pdm_mic_obj *pdm_mic, int16_t* buffer, size_t samples) {
    int filter_stride = (pdm_mic->filter.Fs / 1000);
    samples = (samples / filter_stride) * filter_stride;

    if (samples > pdm_mic->config->sample_buffer_size) {
        samples = pdm_mic->config->sample_buffer_size;
    }

    if (pdm_mic->raw_buffer_write_index == pdm_mic->raw_buffer_read_index) {
        return 0;
    }

    uint8_t* in = pdm_mic->raw_buffer[pdm_mic->raw_buffer_read_index];
    int16_t* out = buffer;

    // get the current buffer index
    pdm_mic->raw_buffer_read_index = (pdm_mic->raw_buffer_read_index + 1) % PDM_RAW_BUFFER_COUNT;

    for (int i = 0; i < samples; i += filter_stride) {
#if PDM_DECIMATION == 64
        Open_PDM_Filter_64(in, out, pdm_mic->filter_volume, &pdm_mic->filter);
#elif PDM_DECIMATION == 128
        Open_PDM_Filter_128(in, out, pdm_mic->filter_volume, &pdm_mic->filter);
#else
        #error "Unsupported PDM_DECIMATION value!"
#endif

        in += filter_stride * (PDM_DECIMATION / 8);
        out += filter_stride;
    }

    

    return samples;
}
