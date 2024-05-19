#ifndef MAIN__H
#define MAIN__H

typedef int32_t usb_audio_sample;

#ifndef AUDIO_RATE_kHz
#define AUDIO_RATE_kHz 8
#endif //AUDIO_RATE_kHz

#define USB_MIC_SAMPLE_BUFFER_SIZE  (AUDIO_RATE_kHz)

#endif //MAIN__H