#ifndef _USB_MICROPHONE_H_
#define _USB_MICROPHONE_H_

#include "tusb.h"
#include "tusb_config.h"

#define AUDIO_SAMPLE_RATE   CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE

typedef void (*usb_microphone_tx_pre_load_cb_t)(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
typedef void (*usb_microphone_tx_post_load_cb_t)(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);


void usb_microphone_init();
void usb_microphone_set_tx_pre_load_handler(usb_microphone_tx_pre_load_cb_t handler);
void usb_microphone_set_tx_post_load_handler(usb_microphone_tx_post_load_cb_t handler);
void usb_microphone_task();

extern bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];                  // +1 for master channel 0
extern uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];          // +1 for master channel 0
extern uint32_t sampFreq;
extern uint8_t clkValid;

#endif
