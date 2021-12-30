cmake_minimum_required(VERSION 3.13)

set (PROJECT fexware)

# Pull in SDK (must be before project)
include(third_party/pico-sdk/external/pico_sdk_import.cmake)

# Pull in FreeRTOS
include(third_party/FreeRTOS/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(${PROJECT} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT}
    src/actions.cc
    src/filesystem.cc
    src/main.cc 
    src/parser.cc
    src/tokenizer.cc
    src/layer.cc

    # USB MSC Filesystem Support (move to lib?)
    third_party/port/cdc_msc/flash.c
    third_party/port/tusb/usb_descriptors.c
    src/tud_usb.c

    # ooFatFs Support (move to lib?)
    third_party/ooFatFs/src/ff.c
    third_party/ooFatFs/src/ffunicode.c
    src/diskio.c

    # FreeRTOS
    third_party/port/FreeRTOS/IdleMemory.c)

target_include_directories(${PROJECT} PRIVATE
    include/
    third_party/port/cdc_msc
    third_party/port/FreeRTOS/
    third_party/port/tusb/
    third_party/ooFatFs/src)

target_link_libraries(${PROJECT}
    hardware_flash
    hardware_i2c
    tinyusb_board
    tinyusb_device
    FreeRTOS-Kernel 
    FreeRTOS-Kernel-Heap1
    pico_bootsel_via_double_reset
    pico_multicore
    pico_stdlib)

target_compile_definitions(${PROJECT} PRIVATE
    PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25)

pico_add_extra_outputs(${PROJECT})
