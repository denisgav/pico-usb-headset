cmake_minimum_required(VERSION 3.12)

# rest of your project
add_executable(usb_speaker_16b
    main.c
    usb_descriptors.c
    usb_speaker.c
)

target_include_directories(usb_speaker_16b PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(usb_speaker_16b PRIVATE tinyusb_device tinyusb_board pico_stdlib pico_i2S_audio pico_volume_ctrl)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(usb_speaker_16b)
