#ifndef MAIN__H
#define MAIN__H

//-------------------------
// I2s defines
//-------------------------
//#define MIC_INMP441

#ifdef MIC_INMP441
    #ifndef MIC_SD
        #define MIC_SD 2 
    #endif //MIC_SD

    #ifndef MIC_SCK
        #define MIC_SCK 3
    #endif //MIC_SCK

    #ifndef MIC_WS
        #define MIC_WS (MIC_SCK+1) // needs to be MIC_SCK +1
    #endif //MIC_WS
#else //MIC_INMP441
    #ifndef MIC_SD
        #define MIC_SD 8 
    #endif //MIC_SD

    #ifndef MIC_SCK
        #define MIC_SCK 9
    #endif //MIC_SCK

    #ifndef MIC_WS
        #define MIC_WS (MIC_SCK+1) // needs to be MIC_SCK +1
    #endif //MIC_WS
#endif //MIC_INMP441

#ifndef MIC_BPS
    #define MIC_BPS 32 // 24 is not valid in this implementation, but INMP441 outputs 24 bits samples
#endif //MIC_BPS

#ifndef MIC_RATE_DEF
    #define MIC_RATE_DEF (16000)
#endif //MIC_RATE_DEF

#ifndef SPK_SD
    #define SPK_SD 5 
#endif //SPK_SD

#ifndef SPK_SCK
    #define SPK_SCK 6
#endif //SPK_SCK

#ifndef SPK_WS
    #define SPK_WS (SPK_SCK+1) // needs to be SPK_SCK +1
#endif //SPK_WS

#ifndef SPK_BPS
    #define SPK_BPS 32
#endif //SPK_BPS

#ifndef SPK_RATE_DEF
    #define SPK_RATE_DEF (48000)
#endif //SPK_RATE_DEF

typedef struct  {
    uint32_t left;
    uint32_t right;
} i2s_32b_audio_sample;

typedef struct  {
    uint16_t left;
    uint16_t right;
} i2s_16b_audio_sample;

#define SAMPLE_BUFFER_SIZE  (48000/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

//-------------------------

#endif //MAIN__H