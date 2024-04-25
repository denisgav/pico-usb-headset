#ifndef MAIN__H
#define MAIN__H

//-------------------------
// I2s defines
//-------------------------
#ifndef SCK
    #define SCK 3
#endif //SCK

#ifndef WS
    #define WS (SCK+1) // needs to be SCK +1
#endif //WS

#ifndef SD
    #define SD 2 // original value 29
#endif //SD

#ifndef BPS
    #define BPS 32 // 24 is not valid in this implementation, but INMP441 outputs 24 bits samples
#endif //BPS

#ifndef RATE
    #define RATE (CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE)
#endif //RATE

typedef struct  {
    uint32_t left;
    uint32_t right;
} i2s_audio_sample;

typedef int32_t usb_audio_sample;

//-------------------------

#endif //MAIN__H