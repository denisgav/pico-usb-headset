#ifndef MAIN__H
#define MAIN__H

//-------------------------
// I2s defines
//-------------------------
#ifndef SCK
    #define SCK 1
#endif //SCK

#ifndef WS
    #define WS (SCK+1) // needs to be SCK +1
#endif //WS

#ifndef SD
    #define SD 0 // original value 29
#endif //SD

#ifndef BPS
    #define BPS 32 // 24 is not valid in this implementation, but INMP441 outputs 24 bits samples
#endif //BPS

#ifndef RATE
    #define RATE (AUDIO_RATE_kHz*1000)
#endif //RATE

typedef struct  {
    uint32_t left;
    uint32_t right;
} i2s_audio_sample;

typedef int32_t usb_audio_sample;

//-------------------------

#endif //MAIN__H