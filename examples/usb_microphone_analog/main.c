
#include "pico/stdlib.h"

#include "main.h"
#include "pico/analog_microphone.h"

#include "usb_microphone.h"

//-------------------------
// Onboard LED
//-------------------------
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
//-------------------------
//-------------------------

//-------------------------
// variables
//-------------------------
// variables
int16_t sample_buffer[USB_MIC_SAMPLE_BUFFER_SIZE];
volatile int samples_read = 0;
//-------------------------


//-------------------------
// callback functions
//-------------------------
void on_usb_microphone_tx_ready();
void on_usb_microphone_tx_done();
void on_analog_samples_ready();
//-------------------------

// configuration
const struct analog_microphone_config config = {
    // GPIO to use for input, must be ADC compatible (GPIO 26 - 28)
    .gpio = 26,

    // bias voltage of microphone in volts
    .bias_voltage = 1.25,

    // sample rate in Hz
    .sample_rate = AUDIO_RATE_kHz*1000,

    // number of samples to buffer
    .sample_buffer_size = USB_MIC_SAMPLE_BUFFER_SIZE,
};

int main(void)
{
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  //printf("hello analog microphone\n");

  // initialize the analog microphone
  if (analog_microphone_init(&config) < 0) {
      //printf("analog microphone initialization failed!\n");
      while (1) { tight_loop_contents(); }
  }

  // set callback that is called when all the samples in the library
  // internal sample buffer are ready for reading
  analog_microphone_set_samples_ready_handler(on_analog_samples_ready);
  
  // start capturing data from the analog microphone
  if (analog_microphone_start() < 0) {
      //printf("PDM microphone start failed!\n");
      while (1) { tight_loop_contents();  }
  }

  // initialize the USB microphone interface
  usb_microphone_init();
  usb_microphone_set_tx_ready_handler(on_usb_microphone_tx_ready);
  usb_microphone_set_tx_done_handler(on_usb_microphone_tx_done);

  // Wait for it to start up

  while (1) {
    // run the USB microphone task continuously
    usb_microphone_task();
  }

  return 0;
}

//-------------------------
// callback functions
//-------------------------

void on_usb_microphone_tx_ready()
{
  // Callback from TinyUSB library when all data is ready
  // to be transmitted.
  
  // Write local buffer to the USB microphone
  if(samples_read != 0){
    gpio_put(LED_PIN, 1);

    usb_microphone_write(&sample_buffer, sizeof(sample_buffer));

    gpio_put(LED_PIN, 0);

    samples_read = 0;
  }
}

void on_usb_microphone_tx_done()
{
  // callback from library when all the samples in the library
  // internal sample buffer are ready for reading 
  samples_read = analog_microphone_read(sample_buffer, USB_MIC_SAMPLE_BUFFER_SIZE);
}

void on_analog_samples_ready()
{
    // callback from library when all the samples in the library
    // internal sample buffer are ready for reading 
    // samples_read = analog_microphone_read(sample_buffer, 256);
}