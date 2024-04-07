
#include "pico/stdlib.h"
#include "pico/machine_i2s.h"

#include "usb_microphone.h"

#include "pico/volume_ctrl.h"
#include "main.h"

//-------------------------
// Onboard LED
//-------------------------
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
//-------------------------
//-------------------------

//-------------------------
// variables
//-------------------------
usb_audio_sample sample_buffer[SAMPLE_BUFFER_SIZE];
//-------------------------


//-------------------------
// callback functions
//-------------------------
void on_usb_microphone_tx_ready();
void on_usb_microphone_tx_done();
//-------------------------

// Pointer to I2S handler
machine_i2s_obj_t* i2s0 = NULL;

usb_audio_sample i2s_to_usb_sample_convert(uint32_t sample, uint32_t volume_db);

int main(void)
{
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  i2s0 = create_machine_i2s(0, SCK, WS, SD, RX, BPS, STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, RATE);

  // initialize the USB microphone interface
  usb_microphone_init();
  usb_microphone_set_tx_ready_handler(on_usb_microphone_tx_ready);
  usb_microphone_set_tx_done_handler(on_usb_microphone_tx_done);

  // Wait for it to start up

  while (1) {
    // run the USB microphone task continuously
    usb_microphone_task();
  }

  return 0;
}

//-------------------------
// callback functions
//-------------------------

void on_usb_microphone_tx_ready()
{
  // Callback from TinyUSB library when all data is ready
  // to be transmitted.
  
  // Write local buffer to the USB microphone
  gpio_put(LED_PIN, 1);

  usb_microphone_write(&sample_buffer, sizeof(sample_buffer));

  gpio_put(LED_PIN, 0);
}

void on_usb_microphone_tx_done()
{
  i2s_audio_sample buffer[SAMPLE_BUFFER_SIZE];
  uint16_t volume_db_left = vol_to_db_convert(mute[0], volume[0]);
  if(i2s0) {
    // Read data from microphone
    int num_bytes_read = machine_i2s_read_stream(i2s0, (void*)&buffer[0], sizeof(buffer));

    if(num_bytes_read >= I2S_RX_FRAME_SIZE_IN_BYTES) {
      int num_of_frames_read = num_bytes_read/I2S_RX_FRAME_SIZE_IN_BYTES;
      for(uint32_t i = 0; i < num_of_frames_read; i++){
          //sample_buffer[i] = buffer[i*2]>>8;
          sample_buffer[i] = i2s_to_usb_sample_convert(buffer[i].left, volume_db_left); // TODO: check this value
      }
    }
  }
}

usb_audio_sample i2s_to_usb_sample_convert(uint32_t sample, uint32_t volume_db)
{
  int64_t sample_tmp = (sample>= 0x800000) ? -(0xFFFFFFFF - sample + 1) : sample;
  sample_tmp = volume_db * sample_tmp;
  sample_tmp = sample_tmp >> 8;
  return sample_tmp;
}