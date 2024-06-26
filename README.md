# Making USB headset using Raspberry pi pico

Capture audio from a microphone on your [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/) or any [RP2040](https://www.raspberrypi.org/products/rp2040/) based board. 🎤


## Hardware

 * RP2040 board
   * [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
 * Microphones
   * PDM
     * [Adafruit PDM MEMS Microphone Breakout](https://www.adafruit.com/product/3492)
   * I2S
     * inmp441
     * [Adafruit I2S MEMS Microphone Breakout - SPH0645LM4H] (https://www.adafruit.com/product/3421)
   * Analog
     * [Electret Microphone Amplifier - MAX9814 with Auto Gain Control] (https://www.adafruit.com/product/1713)
 * DAC
   * PCM5102a

### Default Pinout

#### PDM Microphone

| Raspberry Pi Pico / RP2040 | PDM Microphone |
| --- | --- |
| 3.3V | VCC |
| GND | GND |
| GND | SEL |
| GPIO 18 | PDM1 DAT |
| GPIO 19 | PDM1 CLK |
| GPIO 20 | PDM2 DAT |
| GPIO 21 | PDM2 CLK |

#### INMP441 Microphone

| Raspberry Pi Pico / RP2040 | I2S Microphone |
| --- | --- |
| 3.3V | VCC |
| GND | GND |
| GPIO 14 | SD |
| GPIO 15 | SCK |
| GPIO 16 | WS |

#### SPH0645 Microphone

| Raspberry Pi Pico / RP2040 | I2S Microphone |
| --- | --- |
| 3.3V | VCC |
| GND | GND |
| GPIO 10 | SD |
| GPIO 11 | SCK |
| GPIO 12 | WS |

#### Analog Microphone

| Raspberry Pi Pico / RP2040 | MAX9814 Microphone |
| --- | --- |
| 3.3V | VCC |
| GND | GND |
| GPIO 26 | OUT |

#### PCM5102a DAC

| Raspberry Pi Pico / RP2040 | I2S DAC |
| --- | --- |
| 3.3V | VCC |
| GND | GND |
| GPIO 2 | SD |
| GPIO 3 | SCK |
| GPIO 4 | WS |

GPIO pins are configurable in examples or API.

## Examples

See [examples](examples/) folder.


## Cloning

```sh
git clone https://github.com/denisgav/pico-usb-headset.git
```

## Building

1. [Set up the Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
2. Set `PICO_SDK_PATH`
```sh
export PICO_SDK_PATH=/path/to/pico-sdk
```
3. Create `build` dir, run `cmake` and `make`:
```
mkdir build
cd build
cmake .. -DPICO_BOARD=pico
make
```
4. Copy example `.uf2` to Pico when in BOOT mode.


## Acknowledgements

To create this project, following references were used:
 * The [TinyUSB](https://github.com/hathach/tinyusb) library.
 * Machine I2S  https://github.com/sfera-labs/arduino-pico-i2s-audio
 * SK9822 LED https://github.com/raspberrypi/pico-examples/tree/master/pio/ws2812
 * Microphone library for pico https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico.git 
---
