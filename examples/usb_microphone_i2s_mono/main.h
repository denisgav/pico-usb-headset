#ifndef MAIN__H
#define MAIN__H

//-------------------------
// I2s defines
//-------------------------
//#define MIC_INMP441

#ifdef MIC_INMP441
    #ifndef MIC_SD
        #define MIC_SD 14
    #endif //MIC_SD

    #ifndef MIC_SCK
        #define MIC_SCK 15
    #endif //MIC_SCK

    #ifndef MIC_WS
        #define MIC_WS (MIC_SCK+1) // needs to be MIC_SCK +1
    #endif //MIC_WS
#else //MIC_INMP441
    #ifndef MIC_SD
        #define MIC_SD 10 
    #endif //MIC_SD

    #ifndef MIC_SCK
        #define MIC_SCK 11
    #endif //MIC_SCK

    #ifndef MIC_WS
        #define MIC_WS (MIC_SCK+1) // needs to be MIC_SCK +1
    #endif //MIC_WS
#endif //MIC_INMP441


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