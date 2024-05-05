cmake_minimum_required(VERSION 3.12)

# rest of your project
add_executable(usb_headset
    main.c
    usb_descriptors.c
    usb_headset.c
)

target_include_directories(usb_headset PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(usb_headset PRIVATE tinyusb_device tinyusb_board pico_stdlib pico_multicore pico_i2S_audio pico_volume_ctrl)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(usb_headset)
