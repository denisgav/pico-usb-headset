cmake_minimum_required(VERSION 3.12)

# rest of your project
add_executable(usb_microphone_i2s_mono
    main.c
    usb_descriptors.c
    usb_microphone.c
)

target_include_directories(usb_microphone_i2s_mono PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(usb_microphone_i2s_mono PRIVATE tinyusb_device tinyusb_board pico_stdlib pico_i2S_audio)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(usb_microphone_i2s_mono)
