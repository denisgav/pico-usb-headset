/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Reinhard Panhuber
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/* plot_audio_samples.py requires following modules:
 * $ sudo apt install libportaudio
 * $ pip3 install sounddevice matplotlib
 *
 * Then run
 * $ python3 plot_audio_samples.py
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/machine_i2s.h"

#include "tusb.h"
#include "tusb_config.h"

#include "usb_microphone.h"

#include "pico/volume_ctrl.hpp"



#include "main.h"

//-------------------------
// Onboard LED
//-------------------------
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
//-------------------------
//-------------------------

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
#define SAMPLE_BUFFER_SIZE  (AUDIO_SAMPLE_RATE/1000)



#if CFG_TUD_AUDIO_ENABLE_ENCODING
// Audio test data, each buffer contains 2 channels, buffer[0] for CH0-1, buffer[1] for CH1-2
usb_audio_sample i2s_dummy_buffer[CFG_TUD_AUDIO_FUNC_1_N_TX_SUPP_SW_FIFO][CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX*CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE/1000/CFG_TUD_AUDIO_FUNC_1_N_TX_SUPP_SW_FIFO];
#else
// Audio test data, 4 channels muxed together, buffer[0] for CH0, buffer[1] for CH1, buffer[2] for CH2, buffer[3] for CH3
usb_audio_sample i2s_dummy_buffer[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX*CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE/1000];
#endif

//-------------------------
// callback functions
//-------------------------
void on_usb_microphone_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
//-------------------------

usb_audio_sample i2s_to_usb_sample_convert(uint32_t sample, uint32_t volume_db);

// Pointer to I2S handler
machine_i2s_obj_t* i2s0 = NULL;

/*------------- MAIN -------------*/
int main(void)
{
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  i2s0 = create_machine_i2s(0, SCK, WS, SD, RX, BPS, STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, RATE);

  usb_microphone_init();
  usb_microphone_set_tx_pre_load_handler(on_usb_microphone_tx_pre_load);
  usb_microphone_set_tx_post_load_handler(on_usb_microphone_tx_post_load);

  while (1)
  {
    usb_microphone_task(); // tinyusb device task
  }
}


//-------------------------
// callback functions
//-------------------------
void on_usb_microphone_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  gpio_put(LED_PIN, 1);
#if CFG_TUD_AUDIO_ENABLE_ENCODING
  // Write I2S buffer into FIFO
  for (uint8_t cnt=0; cnt < 2; cnt++)
  {
    tud_audio_write_support_ff(cnt, i2s_dummy_buffer[cnt], AUDIO_SAMPLE_RATE/1000 * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_CHANNEL_PER_FIFO_TX);
  }
#else
  tud_audio_write(i2s_dummy_buffer, AUDIO_SAMPLE_RATE/1000 * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX);
#endif
  gpio_put(LED_PIN, 0);
}

void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  uint16_t volume_db_left = vol_to_db_convert(mute[0], volume[0]);
  uint16_t volume_db_right = vol_to_db_convert(mute[1], volume[1]);

  i2s_audio_sample buffer[SAMPLE_BUFFER_SIZE];
  if(i2s0) {
    // Read data from microphone
    int num_bytes_read = machine_i2s_read_stream(i2s0, (void*)&buffer[0], sizeof(buffer));

    if(num_bytes_read >= I2S_RX_FRAME_SIZE_IN_BYTES) {
      int num_of_frames_read = num_bytes_read/I2S_RX_FRAME_SIZE_IN_BYTES;
      for(uint32_t i = 0; i < num_of_frames_read; i++){
        #if CFG_TUD_AUDIO_ENABLE_ENCODING
          i2s_dummy_buffer[0][i*2] = i2s_to_usb_sample_convert((buffer[i].left), volume_db_left); // TODO: check this value
          i2s_dummy_buffer[0][i*2+1] = i2s_to_usb_sample_convert((buffer[i].right), volume_db_right); // TODO: check this value
          i2s_dummy_buffer[1][i*2] = 0; // TODO: check this value
          i2s_dummy_buffer[1][i*2+1] = 0; // TODO: check this value
        #else
          i2s_dummy_buffer[i*4] = i2s_to_usb_sample_convert((buffer[i].left), volume_db_left); // TODO: check this value
          i2s_dummy_buffer[i*4+1] = i2s_to_usb_sample_convert((buffer[i].right), volume_db_right); // TODO: check this value
          i2s_dummy_buffer[i*4+2] = 0; // TODO: check this value
          i2s_dummy_buffer[i*4+3] = 0; // TODO: check this value
        #endif
      }
    }
  }
}

usb_audio_sample i2s_to_usb_sample_convert(uint32_t sample, uint32_t volume_db)
{
  int64_t sample_tmp = (sample>= 0x800000) ? -(0xFFFFFFFF - sample + 1) : sample;
  sample_tmp = volume_db * sample_tmp;
  sample_tmp = sample_tmp >> 9;//sample_tmp >> 8;
  return sample_tmp;
}