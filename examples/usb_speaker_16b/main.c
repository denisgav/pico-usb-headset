/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jerzy Kasenberg
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

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "usb_speaker.h"

#include "main.h"
#include "pico/machine_i2s.h"
#include "pico/volume_ctrl.h"
#include "pico/default_i2s_board_defines.h"

// Pointer to I2S handler
machine_i2s_obj_t* speaker_i2s0 = NULL;

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTYPES
//--------------------------------------------------------------------+
typedef struct _current_settings_t {
    // Audio controls
  // Current states
  int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];       // +1 for master channel 0
  int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];    // +1 for master channel 0
  int32_t volume_db[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];    // +1 for master channel 0

  uint32_t sample_rate;
  uint8_t resolution;
  uint32_t blink_interval_ms;

  uint16_t samples_in_i2s_frame_min;
  uint16_t samples_in_i2s_frame_max;

  bool mic_muted_by_user;
  bool mic_live_status;

  uint32_t cur_time_ms;

} current_settings_t;

current_settings_t current_settings;

// Buffer for speaker data
//i2s_32b_audio_sample spk_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_32b_audio_sample spk_32b_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_16b_audio_sample spk_16b_i2s_buffer[SAMPLE_BUFFER_SIZE];
int32_t spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4];

void led_blinking_task(void);

int32_t usb_to_i2s_32b_sample_convert(int32_t sample, int32_t volume_db);

int16_t usb_to_i2s_16b_sample_convert(int16_t sample, int32_t volume_db);

void refresh_i2s_connections()
{
  current_settings.samples_in_i2s_frame_min = (current_settings.sample_rate)    /1000;
  current_settings.samples_in_i2s_frame_max = (current_settings.sample_rate+999)/1000;
  current_settings.mic_muted_by_user = false;
  current_settings.cur_time_ms = 0;

  speaker_i2s0 = create_machine_i2s(0, I2S_SPK_SCK, I2S_SPK_WS, I2S_SPK_SD, TX, 
    ((current_settings.resolution == 16) ? 16 : 32), STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, current_settings.sample_rate);
  
  // update_pio_frequency(speaker_i2s0, current_settings.usb_sample_rate);
  // update_pio_frequency(microphone_i2s1, current_settings.usb_sample_rate);
}


void usb_speaker_mute_handler(int8_t bChannelNumber, int8_t mute_in);
void usb_speaker_volume_handler(int8_t bChannelNumber, int16_t volume_in);
void usb_speaker_current_sample_rate_handler(uint32_t current_sample_rate_in);
void usb_speaker_current_resolution_handler(uint8_t current_resolution_in);
void usb_speaker_current_status_set_handler(uint32_t blink_interval_ms_in);

void usb_speaker_tud_audio_rx_done_pre_read_handler(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting);

/*------------- MAIN -------------*/
int main(void)
{
  current_settings.sample_rate  = I2S_SPK_RATE_DEF;
  current_settings.resolution = CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX;
  current_settings.blink_interval_ms = BLINK_NOT_MOUNTED;

  usb_speaker_set_mute_set_handler(usb_speaker_mute_handler);
  usb_speaker_set_volume_set_handler(usb_speaker_volume_handler);
  usb_speaker_set_current_sample_rate_set_handler(usb_speaker_current_sample_rate_handler);
  usb_speaker_set_current_resolution_set_handler(usb_speaker_current_resolution_handler);
  usb_speaker_set_current_status_set_handler(usb_speaker_current_status_set_handler);

  usb_speaker_set_tud_audio_rx_done_pre_read_set_handler(usb_speaker_tud_audio_rx_done_pre_read_handler);
  
  usb_speaker_init();
  refresh_i2s_connections();

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1); i++)
  {
    current_settings.volume[i] = DEFAULT_VOLUME;
    current_settings.mute[i] = 0;
    current_settings.volume_db[i] = vol_to_db_convert(current_settings.mute[i], current_settings.volume[i]);
  }

  while (1) {
    usb_speaker_task();

    current_settings.cur_time_ms = board_millis();
    led_blinking_task();
  }
}

void usb_speaker_mute_handler(int8_t bChannelNumber, int8_t mute_in)
{
  current_settings.mute[bChannelNumber] = mute_in;
  current_settings.volume_db[bChannelNumber] = vol_to_db_convert(current_settings.mute[bChannelNumber], current_settings.volume[bChannelNumber]);
}

void usb_speaker_volume_handler(int8_t bChannelNumber, int16_t volume_in)
{
  current_settings.volume[bChannelNumber] = volume_in;
  current_settings.volume_db[bChannelNumber] = vol_to_db_convert(current_settings.mute[bChannelNumber], current_settings.volume[bChannelNumber]);
}

void usb_speaker_current_sample_rate_handler(uint32_t current_sample_rate_in)
{
  current_settings.sample_rate = current_sample_rate_in;
  refresh_i2s_connections();
}

void usb_speaker_current_resolution_handler(uint8_t current_resolution_in)
{
  current_settings.resolution = current_resolution_in;
  refresh_i2s_connections();
}

void usb_speaker_current_status_set_handler(uint32_t blink_interval_ms_in)
{
  current_settings.blink_interval_ms = blink_interval_ms_in;
}

void usb_speaker_tud_audio_rx_done_pre_read_handler(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
  uint32_t volume_db_master = current_settings.volume_db[0];
  uint32_t volume_db_left = current_settings.volume_db[1];
  uint32_t volume_db_right = current_settings.volume_db[2];

  if(speaker_i2s0 && (current_settings.blink_interval_ms == BLINK_STREAMING)){
    // Speaker data size received in the last frame
    uint16_t usb_spk_data_size = tud_audio_read(spk_buf, n_bytes_received);
    uint16_t usb_sample_count = 0;

    if (current_settings.resolution == 16)
    {
      int16_t *in = (int16_t *) spk_buf;
      usb_sample_count = usb_spk_data_size/4; // 4 bytes per sample 2b left, 2b right

      if(usb_sample_count >= current_settings.samples_in_i2s_frame_min){
        for (int i = 0; i < usb_sample_count; i++) {
          int16_t left = in[i*2 + 0];
          int16_t right = in[i*2 + 1];

          left = usb_to_i2s_16b_sample_convert(left, volume_db_left);
          left = usb_to_i2s_16b_sample_convert(left, volume_db_master);
          right = usb_to_i2s_16b_sample_convert(right, volume_db_right);
          right = usb_to_i2s_16b_sample_convert(right, volume_db_master);
          spk_16b_i2s_buffer[i].left  = left;
          spk_16b_i2s_buffer[i].right = right;
        }
        machine_i2s_write_stream(speaker_i2s0, (void*)&spk_16b_i2s_buffer[0], usb_sample_count*4);
      }
    }
    else if (current_settings.resolution == 24)
    {
      int32_t *in = (int32_t *) spk_buf;
      usb_sample_count = usb_spk_data_size/8; // 8 bytes per sample 4b left, 4b right

      if(usb_sample_count >= current_settings.samples_in_i2s_frame_min){
        for (int i = 0; i < usb_sample_count; i++) {
          int32_t left = in[i*2 + 0];
          int32_t right = in[i*2 + 1];

          left = usb_to_i2s_32b_sample_convert(left, volume_db_left);
          left = usb_to_i2s_32b_sample_convert(left, volume_db_master);
          right = usb_to_i2s_32b_sample_convert(right, volume_db_right);
          right = usb_to_i2s_32b_sample_convert(right, volume_db_master);
          spk_32b_i2s_buffer[i].left  = left;
          spk_32b_i2s_buffer[i].right = right;
        }
        machine_i2s_write_stream(speaker_i2s0, (void*)&spk_32b_i2s_buffer[0], usb_sample_count*8);
      }
    }
  }
}


int32_t usb_to_i2s_32b_sample_convert(int32_t sample, int32_t volume_db)
{
  int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
  sample_tmp = sample_tmp>>15;
  return (int32_t)sample_tmp;
  //return (int32_t)sample;
}

int16_t usb_to_i2s_16b_sample_convert(int16_t sample, int32_t volume_db)
{
  int32_t sample_tmp = (int32_t)sample * (int32_t)volume_db;
  sample_tmp = sample_tmp>>15;
  return (int16_t)sample_tmp;
  //return (int16_t)sample;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  static uint32_t mic_live_status_update_ms = 0;

  // Blink every interval ms
  if (current_settings.cur_time_ms - start_ms < current_settings.blink_interval_ms) return;
  start_ms += current_settings.blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state;
}
