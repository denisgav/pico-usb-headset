/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef _USB_MICROPHONE_H_
#define _USB_MICROPHONE_H_

#include "tusb.h"

#ifndef SAMPLE_RATE
#define SAMPLE_RATE ((CFG_TUD_AUDIO_EP_SZ_IN / CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX)-1) * 1000
#endif

#ifndef SAMPLE_BUFFER_SIZE
#define SAMPLE_BUFFER_SIZE ((CFG_TUD_AUDIO_EP_SZ_IN / CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX)-1)
#endif

typedef void (*usb_microphone_tx_ready_handler_t)(void);
typedef void (*usb_microphone_tx_done_handler_t)(void);

void usb_microphone_init();
void usb_microphone_set_tx_ready_handler(usb_microphone_tx_ready_handler_t handler);
void usb_microphone_set_tx_done_handler(usb_microphone_tx_done_handler_t handler);
void usb_microphone_task();
uint16_t usb_microphone_write(const void * data, uint16_t len);

// Audio controls
// Current states
extern bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; 						// +1 for master channel 0
extern uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; 					// +1 for master channel 0
extern uint32_t sampFreq;
extern uint8_t clkValid;

#endif
