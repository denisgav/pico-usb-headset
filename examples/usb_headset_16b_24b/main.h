#ifndef MAIN__H
#define MAIN__H

//-------------------------
// I2s defines
//-------------------------
typedef int32_t usb_audio_sample;

#define USB_MIC_SAMPLE_BUFFER_SIZE  (I2S_MIC_RATE_DEF/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

#define SAMPLE_BUFFER_SIZE  (48000/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

//-------------------------

#endif //MAIN__H