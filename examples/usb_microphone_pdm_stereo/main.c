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

#include "tusb.h"
#include "tusb_config.h"

#include "usb_microphone.h"

#include "pico/pdm_microphone.h"
#include "pico/volume_ctrl.hpp"

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


// PDM configuration
pdm_microphone_config config_l = {
  .pdm_id = 0,
  .dma_irq = DMA_IRQ_0,
  .pio = pio0,
  .gpio_data = 2,
  .gpio_clk = 3,
  .sample_rate = AUDIO_SAMPLE_RATE,
  .sample_buffer_size = SAMPLE_BUFFER_SIZE,
};

pdm_microphone_config config_r = {
  .pdm_id = 1,
  .dma_irq = DMA_IRQ_0,
  .pio = pio0,
  .gpio_data = 4,
  .gpio_clk = 5,
  .sample_rate = AUDIO_SAMPLE_RATE,
  .sample_buffer_size = SAMPLE_BUFFER_SIZE,
};

pdm_mic_obj* pdm_mic_l;
pdm_mic_obj* pdm_mic_r;

// variables
int16_t sample_buffer_l[SAMPLE_BUFFER_SIZE];
int16_t sample_buffer_r[SAMPLE_BUFFER_SIZE];

typedef int16_t usb_audio_sample;


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
void on_pdm_samples_ready(uint8_t pdm_id);
void on_usb_microphone_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
//-------------------------

usb_audio_sample pdm_to_usb_sample_convert(int16_t sample, uint16_t volume_db);

/*------------- MAIN -------------*/
int main(void)
{
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // initialize and start the PDM microphone
  pdm_mic_l = pdm_microphone_init(&config_l);
  if (pdm_mic_l == NULL) {
      printf("PDM microphone initialization failed!\n");
      while (1) { tight_loop_contents(); }
  }
  pdm_microphone_set_samples_ready_handler(pdm_mic_l, on_pdm_samples_ready);

  pdm_mic_r = pdm_microphone_init(&config_r);
  if (pdm_mic_r == NULL) {
      printf("PDM microphone initialization failed!\n");
      while (1) { tight_loop_contents(); }
  }
  pdm_microphone_set_samples_ready_handler(pdm_mic_r, on_pdm_samples_ready);

  if(pdm_microphone_start(pdm_mic_l) != 0){
    printf("PDM microphone start failed!\n");
    while (1) { tight_loop_contents(); }
  }
  
  if(pdm_microphone_start(pdm_mic_r) != 0){
     printf("PDM microphone start failed!\n");
     while (1) { tight_loop_contents(); }
   }


  usb_microphone_init();
  usb_microphone_set_tx_pre_load_handler(on_usb_microphone_tx_pre_load);
  usb_microphone_set_tx_post_load_handler(on_usb_microphone_tx_post_load);

  gpio_put(LED_PIN, 1);
  while (1)
  {
    usb_microphone_task(); // tinyusb device task
  }
}


//-------------------------
// callback functions
//-------------------------

void on_pdm_samples_ready(uint8_t pdm_id)
{
  // Function is empty 
  // The read would be done in on_usb_microphone_tx_post_load

  // Callback from library when all the samples in the library
  // internal sample buffer are ready for reading.
  //
  // Read new samples into local buffer.

  // if(pdm_id == 0)
  // {
  //   int samples_read = pdm_microphone_read(pdm_mic_l, sample_buffer_l, SAMPLE_BUFFER_SIZE);
  //   for(uint32_t i = 0; i < samples_read; i++){
  //     #if CFG_TUD_AUDIO_ENABLE_ENCODING
  //       i2s_dummy_buffer[0][i*2] = sample_buffer_l[i];
  //       i2s_dummy_buffer[1][i*2] = 0; 
  //     #else
  //       i2s_dummy_buffer[i*4] = sample_buffer_l[i];
  //       i2s_dummy_buffer[i*4+2] = 0;
  //     #endif
  //   }
  // }
  // else
  // {
  //   if(pdm_id == 1)
  //   {
  //     int samples_read = pdm_microphone_read(pdm_mic_r, sample_buffer_r, SAMPLE_BUFFER_SIZE);
  //     for(uint32_t i = 0; i < samples_read; i++){
  //     #if CFG_TUD_AUDIO_ENABLE_ENCODING
  //       i2s_dummy_buffer[0][i*2+1] = sample_buffer_r[i];
  //       i2s_dummy_buffer[1][i*2+1] = 0; 
  //     #else
  //       i2s_dummy_buffer[i*4+1] = sample_buffer_r[i];
  //       i2s_dummy_buffer[i*4+3] = 0;
  //     #endif
  //     }
  //   } 
  // }
}

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

  int samples_read = pdm_microphone_read(pdm_mic_l, sample_buffer_l, SAMPLE_BUFFER_SIZE);
  for(uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++){
  #if CFG_TUD_AUDIO_ENABLE_ENCODING
    i2s_dummy_buffer[0][i*2] = (i<samples_read) ? pdm_to_usb_sample_convert(sample_buffer_l[i], volume_db_left) : 0;
    i2s_dummy_buffer[1][i*2] = 0; 
  #else
    i2s_dummy_buffer[i*4] = (i<samples_read) ? pdm_to_usb_sample_convert(sample_buffer_l[i], volume_db_left) : 0;
    i2s_dummy_buffer[i*4+2] = 0;
  #endif
  }
    
  samples_read = pdm_microphone_read(pdm_mic_r, sample_buffer_r, SAMPLE_BUFFER_SIZE);
  for(uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++){
  #if CFG_TUD_AUDIO_ENABLE_ENCODING
    i2s_dummy_buffer[0][i*2+1] = (i<samples_read) ? pdm_to_usb_sample_convert(sample_buffer_r[i], volume_db_right) : 0;
    i2s_dummy_buffer[1][i*2+1] = 0; 
  #else
    i2s_dummy_buffer[i*4+1] = (i<samples_read) ? pdm_to_usb_sample_convert(sample_buffer_r[i], volume_db_right) : 0;
    i2s_dummy_buffer[i*4+3] = 0;
  #endif
  }  
}

usb_audio_sample pdm_to_usb_sample_convert(int16_t sample, uint16_t volume_db)
{
  int32_t sample_tmp = (uint32_t)(volume_db) * (uint32_t)(sample);
  sample_tmp = sample_tmp >> 15;

  return sample_tmp;
}