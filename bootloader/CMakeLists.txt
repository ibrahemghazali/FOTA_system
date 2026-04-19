cmake_minimum_required(VERSION 4.2)

# -------------------
# Tell CMake this is bare-metal (no OS)
# -------------------
set(CMAKE_SYSTEM_NAME Generic)

# -------------------
# Project
# -------------------



# -------------------
# Compilers
# -------------------
set(CMAKE_C_COMPILER arm-none-eabi-gcc CACHE STRING "C compiler" FORCE)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++ CACHE STRING "CPP compiler" FORCE) 
set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE STRING "arm-objcopy" FORCE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(COMMON_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -Os
    -g
    -ffunction-sections
    -fdata-sections
)
add_compile_options(${COMMON_FLAGS})
add_link_options(${COMMON_FLAGS})
project(bootloader_project C ASM)

# -------------------
# Sources
# -------------------
file(GLOB_RECURSE SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
string(REGEX REPLACE "^build/[^;]+;?" "" SRCS "${SRCS}")

list(FILTER SRCS EXCLUDE REGEX ".*/Template/.*|.*_template\\.c$|.*/system_stm32f4xx\\.c$|.*CMakeCCompilerId\\.c$")
#list(FILTER SRCS EXCLUDE REGEX "Template/ARMv8-M")
#list(FILTER ALL_SRC EXCLUDE REGEX ".*stm32f4xx_hal_timebase_rtc_.*\\.c$")
# -------------------
# Include Paths
# -------------------
set(INCLUDE_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Core/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ${CMAKE_CURRENT_SOURCE_DIR}/bootloader/Inc

)


set(SYSTEM_FILE
    Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
)
# -------------------
# Linker script
# -------------------
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F401XX_FLASH.ld)

# -------------------
# Build executable
# -------------------
# -------------------
# Build executable
# -------------------
set(STARTUP_FILE startup_stm32f401xc.s)
add_executable(${PROJECT_NAME}.elf
    main.c
    ${SYSTEM_FILE}
    ${SRCS}
    ${STARTUP_FILE}
)

# Include paths
target_include_directories(${PROJECT_NAME}.elf PUBLIC ${INCLUDE_PATHS})

# C flags

# target_compile_options(${PROJECT_NAME}.elf PUBLIC -g  -mcpu=cortex-m4 -mthumb  -O0 -g -Wall -mfpu=fpv4-sp-d16  -mfloat-abi=hard)

# ASM flags (startup file)
set_source_files_properties(startup_stm32f401xc.s PROPERTIES
    COMPILE_FLAGS "-x assembler-with-cpp -mcpu=cortex-m4   -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# Force FPU flags on FreeRTOS portable files
set_source_files_properties(
    FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
    PROPERTIES COMPILE_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -O0 -g"
)


#Linker
target_link_options(${PROJECT_NAME}.elf PUBLIC
    -T ${LINKER_SCRIPT} -Wl,-Map=${PROJECT_NAME}.map -Wl,--gc-sections  -mfpu=fpv4-sp-d16
    -mfloat-abi=hard)
# -------------------
# Post-build: generate .bin and .hex
# -------------------
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMENT "Generating .bin and .hex files"
)

# -------------------
# Custom flash target
# -------------------
add_custom_target(flash
    DEPENDS ${PROJECT_NAME}.elf
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND st-flash write ${PROJECT_NAME}.bin 0x08000000
    COMMENT "Flashing ${PROJECT_NAME} to STM32F4"
     USES_TERMINAL
) 