
#include "pico/stdlib.h"
#include "pico/machine_i2s.h"

#include "main.h"
#include "pico/default_i2s_board_defines.h"
//#include "pico/dc_offset_filter.h"

#include "usb_microphone.h"

//-------------------------
// Onboard LED
//-------------------------
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
//-------------------------
//-------------------------

//-------------------------
// variables
//-------------------------
usb_audio_sample sample_buffer[USB_MIC_SAMPLE_BUFFER_SIZE];
//-------------------------


//-------------------------
// callback functions
//-------------------------
void on_usb_microphone_tx_ready();
void on_usb_microphone_tx_done();
//-------------------------

// Pointer to I2S handler
machine_i2s_obj_t* i2s0 = NULL;

//dc_offset_filter_t dc_offset_filter;

usb_audio_sample mic_i2s_to_usb_sample_convert(uint32_t sample_idx, uint32_t sample);

int main(void)
{
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  //dc_offset_filter_init(&dc_offset_filter, 4*48*1000);

  i2s0 = create_machine_i2s(0, I2S_MIC_SCK, I2S_MIC_WS, I2S_MIC_SD, RX, I2S_MIC_BPS, STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, I2S_MIC_RATE_DEF);

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
  i2s_32b_audio_sample buffer[USB_MIC_SAMPLE_BUFFER_SIZE];
  if(i2s0) {
    // Read data from microphone
    int num_bytes_read = machine_i2s_read_stream(i2s0, (void*)&buffer[0], sizeof(buffer));

    if(num_bytes_read >= I2S_RX_FRAME_SIZE_IN_BYTES) {
      int num_of_frames_read = num_bytes_read/I2S_RX_FRAME_SIZE_IN_BYTES;
      for(uint32_t i = 0; i < num_of_frames_read; i++){
          //sample_buffer[i] = buffer[i*2]>>8;
          sample_buffer[i] = mic_i2s_to_usb_sample_convert(i, buffer[i].left); // TODO: check this value
      }
    }
  }
}

#ifdef I2S_MIC_INMP441
// Microphone INMP441 is used
usb_audio_sample mic_i2s_to_usb_sample_convert(uint32_t sample_idx, uint32_t sample)
{
  int32_t sample_tmp = sample;
  return sample_tmp; //<<4;
}
#else //I2S_MIC_INMP441
// Microphone SPH0645 is used
usb_audio_sample mic_i2s_to_usb_sample_convert(uint32_t sample_idx, uint32_t sample)
{
  int32_t sample_tmp = (sample!= 0) ? (sample - I2S_MIC_SPH_DC_OFFSET) : 0;
  // Mean value calculations:
  //sample_tmp = dc_offset_filter_main(&dc_offset_filter, sample_tmp, (sample_idx == 0));

  // Return shifted value:
  return sample_tmp; //<<4;
}
#endif //I2S_MIC_INMP441