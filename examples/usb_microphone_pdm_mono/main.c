/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * This examples creates a USB Microphone device using the TinyUSB
 * library and captures data from a PDM microphone using a sample
 * rate of 16 kHz, to be sent the to PC.
 * 
 * The USB microphone code is based on the TinyUSB audio_test example.
 * 
 * https://github.com/hathach/tinyusb/tree/master/examples/device/audio_test
 */

#include "pico/pdm_microphone.h"

#include "usb_microphone.h"
#include "pico/volume_ctrl.h"

typedef uint16_t usb_audio_sample;

//-------------------------
// Onboard LED
//-------------------------
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
//-------------------------
//-------------------------

// configuration
pdm_microphone_config config = {
  .pdm_id = 0,
  .dma_irq = DMA_IRQ_0,
  .pio = pio0,
  .gpio_data = 2,
  .gpio_clk = 3,
  .sample_rate = SAMPLE_RATE,
  .sample_buffer_size = SAMPLE_BUFFER_SIZE,
};
pdm_mic_obj* pdm_mic;

// variables
uint16_t pdm_sample_buffer[SAMPLE_BUFFER_SIZE];
usb_audio_sample usb_sample_buffer[SAMPLE_BUFFER_SIZE];

// callback functions
void on_pdm_samples_ready(uint8_t pdm_id);
void on_usb_microphone_tx_ready();
void on_usb_microphone_tx_done();

usb_audio_sample pdm_to_usb_sample_convert(uint16_t sample, uint16_t volume_db);

int main(void)
{
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // initialize and start the PDM microphone
  pdm_mic = pdm_microphone_init(&config);
  if (pdm_mic == NULL) {
      printf("PDM microphone initialization failed!\n");
      while (1) { tight_loop_contents(); }
  }
  pdm_microphone_set_samples_ready_handler(pdm_mic, on_pdm_samples_ready);
  pdm_microphone_start(pdm_mic);

  // initialize the USB microphone interface
  usb_microphone_init();
  usb_microphone_set_tx_ready_handler(on_usb_microphone_tx_ready);
  usb_microphone_set_tx_done_handler(on_usb_microphone_tx_done);

  gpio_put(LED_PIN, 1);

  while (1) {
    // run the USB microphone task continuously
    usb_microphone_task();
  }

  return 0;
}

void on_pdm_samples_ready(uint8_t pdm_id)
{
  // Callback from library when all the samples in the library
  // internal sample buffer are ready for reading.
  //
  // Read new samples into local buffer.
  // uint16_t volume_db_left = vol_to_db_convert(mute[0], volume[0]);

  
}

void on_usb_microphone_tx_ready()
{
  // Callback from TinyUSB library when all data is ready
  // to be transmitted.
  //
  // Write local buffer to the USB microphone

  gpio_put(LED_PIN, 1);

  usb_microphone_write(&usb_sample_buffer, sizeof(usb_sample_buffer));

  gpio_put(LED_PIN, 0);

}

void on_usb_microphone_tx_done()
{
  uint16_t volume_db_left = vol_to_db_convert(mute[0], volume[0]);

  int samples_read = pdm_microphone_read(pdm_mic, pdm_sample_buffer, SAMPLE_BUFFER_SIZE);
  for(uint16_t sample_idx = 0; sample_idx < SAMPLE_BUFFER_SIZE; sample_idx++)
  {
    if(sample_idx < samples_read)
    {
      usb_sample_buffer[sample_idx] = pdm_to_usb_sample_convert(pdm_sample_buffer[sample_idx], volume_db_left);
    }
    else
    {
      usb_sample_buffer[sample_idx] = 0;
    }
  }
}

usb_audio_sample pdm_to_usb_sample_convert(uint16_t sample, uint16_t volume_db)
{
  uint32_t sample_tmp = (uint32_t)(volume_db) * (uint32_t)(sample);
  sample_tmp = sample_tmp >> 15;

  return sample_tmp;
}