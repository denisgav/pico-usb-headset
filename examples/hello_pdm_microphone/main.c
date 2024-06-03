/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * This examples captures data from a PDM microphone using a sample
 * rate of 8 kHz and prints the sample values over the USB serial
 * connection.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/pdm_microphone.h"
#include "tusb.h"

#define AUDIO_SAMPLE_RATE 16000
#define SAMPLE_BUFFER_SIZE  (AUDIO_SAMPLE_RATE/1000)

// cat /dev/ttyACM0 | xxd -r -p | aplay -r16000 -c1 -fS16_BE

// configuration
pdm_microphone_config config = {
    // PDM ID
    .pdm_id = 0,

    .dma_irq = DMA_IRQ_0,

    // PIO
    .pio = pio0,

    // GPIO pin for the PDM DAT signal
    .gpio_data = 18,

    // GPIO pin for the PDM CLK signal
    .gpio_clk = 19,

    // sample rate in Hz
    .sample_rate = AUDIO_SAMPLE_RATE,

    // number of samples to buffer
    .sample_buffer_size = SAMPLE_BUFFER_SIZE,
};

pdm_mic_obj* pdm_mic;

// variables
int16_t sample_buffer[SAMPLE_BUFFER_SIZE];

volatile bool data_valid = false;

void on_pdm_samples_ready(uint8_t pdm_id)
{
    // callback from library when all the samples in the library
    // internal sample buffer are ready for reading 
    //samples_read = pdm_microphone_read(pdm_mic, sample_buffer, 256);
    data_valid = true;
}

int main( void )
{
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }

    printf("hello PDM microphone\n");

    // initialize the PDM microphone
    pdm_mic = pdm_microphone_init(&config);

    if (pdm_mic == NULL) {
        printf("PDM microphone initialization failed!\n");
        while (1) { tight_loop_contents(); }
    }

    // set callback that is called when all the samples in the library
    // internal sample buffer are ready for reading
    pdm_microphone_set_samples_ready_handler(pdm_mic, on_pdm_samples_ready);
    
     // start capturing data from the PDM microphone
    if (pdm_microphone_start(pdm_mic) != 0) {
        printf("PDM microphone start failed!\n");
        while (1) { tight_loop_contents(); }
    }

    while (1) {
        if(data_valid == true)
        {
            data_valid = false;
            // wait for new samples
            int samples_read = pdm_microphone_read(pdm_mic, sample_buffer, SAMPLE_BUFFER_SIZE);
            //printf("samples_read = %d\n", samples_read);

            if(samples_read != 0)
            {    
                //unsigned long currentMillis = time_us_64();
                // loop through any new collected samples
                for (int i = 0; i < samples_read; i++) {
                    //printf("%d milis [%0d] -> %d\n", currentMillis, i, sample_buffer[i]);
                    printf("%.4x\n", sample_buffer[i]);
                }
            }
        }
        else
        {
            tight_loop_contents();
        }
    }

    return 0;
}
