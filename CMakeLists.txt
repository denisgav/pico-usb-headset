cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()

include(pico_sdk_import.cmake)

project(pico_microphone)

# initialize the Pico SDK
pico_sdk_init()

###################################################
# Volume control library
###################################################
add_library(pico_volume_ctrl INTERFACE)

target_sources(pico_volume_ctrl INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/volume_ctrl.c
)

target_include_directories(pico_volume_ctrl INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/include
)

target_link_libraries(pico_volume_ctrl INTERFACE pico_stdlib)
###################################################

###################################################
# PDM microphone library
###################################################

add_library(pico_pdm_microphone INTERFACE)

target_sources(pico_pdm_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/pdm_microphone.c
    ${CMAKE_CURRENT_LIST_DIR}/src/OpenPDM2PCM/OpenPDMFilter.c
)

target_include_directories(pico_pdm_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/include
    ${CMAKE_CURRENT_LIST_DIR}/src/OpenPDM2PCM
)

pico_generate_pio_header(pico_pdm_microphone ${CMAKE_CURRENT_LIST_DIR}/src/pdm_microphone.pio)

target_link_libraries(pico_pdm_microphone INTERFACE pico_stdlib hardware_dma hardware_pio)
###################################################

###################################################
# I2S microphone library
###################################################

add_library(pico_i2S_audio INTERFACE)

target_sources(pico_i2S_audio INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/machine_i2s.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ring_buf.c
)

target_include_directories(pico_i2S_audio INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/include
)

pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/src/audio_i2s_tx_16b.pio)
pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/src/audio_i2s_tx_32b.pio)
pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/src/audio_i2s_rx_32b.pio)

target_link_libraries(pico_i2S_audio INTERFACE pico_stdlib hardware_dma hardware_pio)
###################################################

add_subdirectory("examples/hello_pdm_microphone")
add_subdirectory("examples/hello_i2s_microphone")

add_subdirectory("examples/usb_microphone_pdm_mono")
add_subdirectory("examples/usb_microphone_pdm_stereo")

add_subdirectory("examples/usb_microphone_i2s_mono")
add_subdirectory("examples/usb_microphone_i2s_stereo")

add_subdirectory("examples/usb_headset")
