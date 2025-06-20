cmake_minimum_required(VERSION 3.13)
project(QS_UV_K6_FIRMWARE C ASM)

# ====== Configuration ======
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(CPU_FLAGS "-mcpu=cortex-m0")
set(COMMON_C_FLAGS "-Os -Wall -Wextra -Werror -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -std=c2x -MMD")
set(CMAKE_C_FLAGS "${CPU_FLAGS} ${COMMON_C_FLAGS}")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS}")

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/firmware.ld)
set(LINKER_FLAGS "-T${LINKER_SCRIPT} -nostartfiles -Wl,--gc-sections -z noexecstack --specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} ${LINKER_FLAGS}")

# ====== Options ======
option(ENABLE_UART "Enable UART" ON)
option(ENABLE_AIRCOPY "Enable Aircopy" OFF)
option(ENABLE_FMRADIO "Enable FM Radio" ON)
option(ENABLE_SPECTRUM "Enable Spectrum" ON)
# 添加更多宏定义选项...

# ====== Include Directories ======
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/libcpu
    ${CMAKE_SOURCE_DIR}/external/libcpu/ARMCM0/Include
)

# ====== Preprocessor Defines ======
add_compile_definitions(
    PRINTF_INCLUDE_CONFIG_H
    AUTHOR_STRING=\"JasonLee\"
    VERSION_STRING=\"V100\" # 可扩展为自动 Git 版本
)

# Feature defines
if(ENABLE_UART)
    add_compile_definitions(ENABLE_UART)
endif()
if(ENABLE_AIRCOPY)
    add_compile_definitions(ENABLE_AIRCOPY)
endif()
if(ENABLE_FMRADIO)
    add_compile_definitions(ENABLE_FMRADIO)
endif()
if(ENABLE_SPECTRUM)
    add_compile_definitions(ENABLE_SPECTRUM)
endif()

# ====== Source Collection ======
file(GLOB_RECURSE C_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/app/*.c
    ${CMAKE_SOURCE_DIR}/driver/*.c
    ${CMAKE_SOURCE_DIR}/misc/*.c
    ${CMAKE_SOURCE_DIR}/ui/*.c
    ${CMAKE_SOURCE_DIR}/helper/*.c
    ${CMAKE_SOURCE_DIR}/external/printf/*.c
)

set(ASM_SOURCES ${CMAKE_SOURCE_DIR}/start.S)

# ====== Output ======
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
add_executable(firmware ${C_SOURCES} ${ASM_SOURCES})

# ====== Post-build binary conversion ======
add_custom_command(TARGET firmware POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary firmware firmware.bin
    COMMENT "Generating firmware.bin"
)

add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Size info:"
    COMMAND arm-none-eabi-size firmware
)
