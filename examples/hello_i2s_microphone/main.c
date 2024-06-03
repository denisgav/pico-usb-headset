/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * This examples captures data from a PDM microphone using a sample
 * rate of 8 kHz and prints the sample values over the USB serial
 * connection.
 */

#include "pico/stdlib.h"
#include "pico/machine_i2s.h"
#include "pico/default_i2s_board_defines.h"

// To play use following command:
//cat /dev/ttyACM0 | xxd -r -p | aplay -r8000 -c2 -fS32_BE

// Pointer to I2S handler
machine_i2s_obj_t* i2s0 = NULL;

int main() {
    stdio_init_all();
    i2s0 = create_machine_i2s(0, I2S_MIC_SCK, I2S_MIC_WS, I2S_MIC_SD, RX, I2S_MIC_BPS, STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, I2S_MIC_RATE_DEF);
    i2s_32b_audio_sample buffer[I2S_MIC_RATE_DEF/1000];
    while (true) {
        int num_bytes_read = machine_i2s_read_stream(i2s0, (void*)&buffer[0], sizeof(buffer));
        if(num_bytes_read >= I2S_RX_FRAME_SIZE_IN_BYTES) {
            int num_of_frames_read = num_bytes_read/I2S_RX_FRAME_SIZE_IN_BYTES;
            for(uint32_t i = 0; i < num_of_frames_read; i++){
                #ifdef I2S_MIC_INMP441
                printf("INMP. #%d: l %.8x, r %.8x\n", i, buffer[i].left, buffer[i].right);
                #else //I2S_MIC_INMP441
                printf("SPH. #%d: l %.8x, r %.8x\n", i, buffer[i].left, buffer[i].right);
                #endif //I2S_MIC_INMP441
            }
        }
    }
}
