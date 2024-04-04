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

#include "pico/stdlib.h"
#include "pico/machine_i2s.h"

#include "main.h"

// To play use following command:
//cat /dev/ttyACM0 | xxd -r -p | aplay -r8000 -c2 -fS32_BE

int main() {
    stdio_init_all();
    machine_i2s_obj_t* i2s0 = create_machine_i2s(0, SCK, WS, SD, RX, BPS, STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, RATE);
    int32_t buffer[I2S_RX_FRAME_SIZE_IN_BYTES /4];
    while (true) {
        machine_i2s_read_stream(i2s0, (void*)&buffer[0], I2S_RX_FRAME_SIZE_IN_BYTES);
        printf("%.8x\n", buffer[0]);
        printf("%.8x\n", buffer[1]);
    }
}
