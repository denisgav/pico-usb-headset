#ifndef DEFAULT_I2S_BOARD_DEFINES__H
#define DEFAULT_I2S_BOARD_DEFINES__H


//-------------------------
// I2s defines
//-------------------------
//#define I2S_MIC_INMP441

#ifdef I2S_MIC_INMP441
    #ifndef I2S_MIC_SD
        #define I2S_MIC_SD 14
    #endif //I2S_MIC_SD

    #ifndef I2S_MIC_SCK
        #define I2S_MIC_SCK 15
    #endif //I2S_MIC_SCK

    #ifndef I2S_MIC_WS
        #define I2S_MIC_WS (I2S_MIC_SCK+1) // needs to be I2S_MIC_SCK +1
    #endif //I2S_MIC_WS
#else //I2S_MIC_INMP441
    #ifndef I2S_MIC_SD
        #define I2S_MIC_SD 10 
    #endif //I2S_MIC_SD

    #ifndef I2S_MIC_SCK
        #define I2S_MIC_SCK 11
    #endif //I2S_MIC_SCK

    #ifndef I2S_MIC_WS
        #define I2S_MIC_WS (I2S_MIC_SCK+1) // needs to be I2S_MIC_SCK +1
    #endif //I2S_MIC_WS
#endif //I2S_MIC_INMP441

#ifndef I2S_MIC_BPS
    #define I2S_MIC_BPS 32 // 24 is not valid in this implementation, but INMP441 outputs 24 bits samples
#endif //I2S_MIC_BPS

#ifndef I2S_MIC_RATE_DEF
    #define I2S_MIC_RATE_DEF (16000)
#endif //I2S_MIC_RATE_DEF

#ifndef I2S_SPK_SD
    #define I2S_SPK_SD 2 
#endif //I2S_SPK_SD

#ifndef I2S_SPK_SCK
    #define I2S_SPK_SCK 3
#endif //I2S_SPK_SCK

#ifndef I2S_SPK_WS
    #define I2S_SPK_WS (I2S_SPK_SCK+1) // needs to be SPK_SCK +1
#endif //I2S_SPK_WS

#ifndef I2S_SPK_BPS
    #define I2S_SPK_BPS 32
#endif //I2S_SPK_BPS

#ifndef I2S_SPK_RATE_DEF
    #define I2S_SPK_RATE_DEF (48000)
#endif //I2S_SPK_RATE_DEF

typedef struct  {
    uint32_t left;
    uint32_t right;
} i2s_32b_audio_sample;

typedef struct  {
    uint16_t left;
    uint16_t right;
} i2s_16b_audio_sample;

//-------------------------

#endif //DEFAULT_I2S_BOARD_DEFINES__H