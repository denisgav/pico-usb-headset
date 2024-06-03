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

usb_audio_sample mic_i2s_to_usb_sample_convert(uint32_t ch_id, uint32_t sample_idx, uint32_t sample);

// Pointer to I2S handler
machine_i2s_obj_t* i2s0 = NULL;

//dc_offset_filter_t dc_offset_filter[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX];

/*------------- MAIN -------------*/
int main(void)
{
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // for(int ch_id=0; ch_id < CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX; ch_id++){
  //   dc_offset_filter_init(&dc_offset_filter[ch_id], 4*48*1000);
  // }
  

  i2s0 = create_machine_i2s(0, I2S_MIC_SCK, I2S_MIC_WS, I2S_MIC_SD, RX, I2S_MIC_BPS, STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, I2S_MIC_RATE_DEF);

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
  i2s_32b_audio_sample buffer[USB_MIC_SAMPLE_BUFFER_SIZE];
  if(i2s0) {
    // Read data from microphone
    int num_bytes_read = machine_i2s_read_stream(i2s0, (void*)&buffer[0], sizeof(buffer));

    if(num_bytes_read >= I2S_RX_FRAME_SIZE_IN_BYTES) {
      int num_of_frames_read = num_bytes_read/I2S_RX_FRAME_SIZE_IN_BYTES;
      for(uint32_t i = 0; i < num_of_frames_read; i++){
        #if CFG_TUD_AUDIO_ENABLE_ENCODING
          i2s_dummy_buffer[0][i*2] = mic_i2s_to_usb_sample_convert(0, i, (buffer[i].left)); // TODO: check this value
          i2s_dummy_buffer[0][i*2+1] = mic_i2s_to_usb_sample_convert(1, i, (buffer[i].right)); // TODO: check this value
          i2s_dummy_buffer[1][i*2] = 0; // TODO: check this value
          i2s_dummy_buffer[1][i*2+1] = 0; // TODO: check this value
        #else
          i2s_dummy_buffer[i*4] = mic_i2s_to_usb_sample_convert(0, i, (buffer[i].left)); // TODO: check this value
          i2s_dummy_buffer[i*4+1] = mic_i2s_to_usb_sample_convert(1, i, (buffer[i].right)); // TODO: check this value
          i2s_dummy_buffer[i*4+2] = 0; // TODO: check this value
          i2s_dummy_buffer[i*4+3] = 0; // TODO: check this value
        #endif
      }
    }
  }
}

#ifdef I2S_MIC_INMP441
// Microphone INMP441 is used
usb_audio_sample mic_i2s_to_usb_sample_convert(uint32_t ch_id, uint32_t sample_idx, uint32_t sample)
{
  int32_t sample_tmp = sample;
  return sample_tmp; //<<4;
}
#else //I2S_MIC_INMP441
// Microphone SPH0645 is used
usb_audio_sample mic_i2s_to_usb_sample_convert(uint32_t ch_id, uint32_t sample_idx, uint32_t sample)
{
  int32_t sample_tmp = (sample!= 0) ? (sample - I2S_MIC_SPH_DC_OFFSET) : 0;
  // Mean value calculations:
  //sample_tmp = dc_offset_filter_main(&dc_offset_filter, sample_tmp, (sample_idx == 0));

  // Return shifted value:
  return sample_tmp; //<<4;
}
#endif //I2S_MIC_INMP441