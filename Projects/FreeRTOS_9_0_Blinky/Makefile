######################################
# Makefile for STM32 Microcontroller
# GNU GCC based
######################################

######################################
# target
######################################
TARGET = STM32F769I_DISCOVERY

######################################
#JTAG and environment configuration
######################################
GCC_BIN           ?= ~/ARM/Utilities/gcc-arm-none-eabi-7-2017-q4-major/bin
GCC_PREFIX        ?= arm-none-eabi

OPENOCD           ?= openocd
OPENOCD_INTERFACE ?= interface/stlink-v2-1.cfg
OPENOCD_CMDS      ?=
OPENOCD_TARGET    ?= target/stm32f7x.cfg

JLINK             ?= JLinkExe
JLINK_DEVICE      ?= STM32F769NI
JLINK_SPEED       ?= 1000
JLINK_INTERFACE   ?= SWD

######################################
#Choose Debugger
#-SEGGER_JLINK enables Systemview
#- Else default = OPENOCD
######################################
DEBUGGER          ?= SEGGER_JLINK

######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -O0

#######################################
# pathes
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
C_SOURCES = \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma2d.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dsi.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_eth.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_ltdc.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_ltdc_ex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sdram.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_fmc.c \
  ../../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rtc.c \
  ../../../FreeRTOS_9_0/Source/croutine.c \
  ../../../FreeRTOS_9_0/Source/list.c \
  ../../../FreeRTOS_9_0/Source/portable/GCC/ARM_CM7/r0p1/port.c \
  ../../../FreeRTOS_9_0/Source/portable/MemMang/heap_4.c \
  ../../../FreeRTOS_9_0/Source/queue.c \
  ../../../FreeRTOS_9_0/Source/tasks.c \
  ../../../FreeRTOS_9_0/Source/timers.c \
  ../../Utilities/Log/lcd_log.c \
  Src/main.c \
  Src/stm32f7xx_hal_msp.c \
  Src/stm32f7xx_hal_timebase_tim.c \
  Src/stm32f7xx_it.c \
  Src/system_stm32f7xx.c \
  syscalls.c \
  ../../Drivers/BSP/Components/mfxstm32l152/mfxstm32l152.c \
  ../../Drivers/BSP/Components/otm8009a/otm8009a.c \
  ../../Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery.c \
  ../../Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_lcd.c \
  ../../Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_sdram.c
ASM_SOURCES = \
  startup_stm32f769xx.s

#######################################
# binaries
#######################################
CC = $(GCC_BIN)/$(GCC_PREFIX)-gcc
AS = $(GCC_BIN)/$(GCC_PREFIX)-gcc -x assembler-with-cpp
CP = $(GCC_BIN)/$(GCC_PREFIX)-objcopy
AR = $(GCC_BIN)/$(GCC_PREFIX)-ar
SZ = $(GCC_BIN)/$(GCC_PREFIX)-size -A -x
OBJDUMP = $(GCC_BIN)/$(GCC_PREFIX)-objdump
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# macros for gcc
AS_DEFS =
#C_DEFS = -DUSE_HAL_DRIVER -D_TIMEVAL_DEFINED -D_SYS_TIME_H_  -DSTM32F769xx -DUSE_STM32F769I_DISCO
C_DEFS = -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F769xx -DARM_MATH_CM7 -DUSE_STM32F769I_DISCO -D_TIMEVAL_DEFINED -D_SYS_TIME_H_
# includes for gcc
AS_INCLUDES =
C_INCLUDES = -I Inc
C_INCLUDES += -I../../Drivers/CMSIS/Device/ST/STM32F7xx/Include
C_INCLUDES += -I../../Drivers/CMSIS/Include
C_INCLUDES += -I../../Drivers/STM32F7xx_HAL_Driver/Inc
C_INCLUDES += -I../../Drivers/BSP/STM32F769I-Discovery
C_INCLUDES += -I../../Drivers/BSP/Components/Common
C_INCLUDES += -I../../Drivers/BSP/Components
#C_INCLUDES += -I../../../FreeRTOS_9_0/Source/portable/GCC/ARM_CM7/r0p1
C_INCLUDES += -I../../../FreeRTOS_9_0/Source/portable/GCC/ARM_CM4F
C_INCLUDES += -I../../../FreeRTOS_9_0/Source/include
C_INCLUDES += -I../../../FreeRTOS_9_0/Source/CMSIS_RTOS
C_INCLUDES += -I../../Utilities
C_INCLUDES += -I../../Utilities/Log
# compile gcc flags
ASFLAGS = -mthumb -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -fno-builtin
CFLAGS = -mthumb -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -fno-builtin
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
# Generate dependency information
CFLAGS += -MD -MP -MF .dep/$(@F).d

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32F769NIHx_FLASH.ld
# libraries
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = -mthumb -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).lss

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.lss: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(OBJDUMP) -S $< > $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir -p $@		

#######################################
# Flash/debug the MCU

ifeq ($(DEBUGGER),SEGGER_JLINK)

flash:
	JLinkExe -if $(JLINK_INTERFACE) -device $(JLINK_DEVICE) -speed $(JLINK_SPEED) -CommanderScript .jlink/flash.jlink

reset:
	JLinkExe -if $(JLINK_INTERFACE) -device $(JLINK_DEVICE) -speed $(JLINK_SPEED) -CommanderScript .jlink/reset.jlink

jlink:
	JLinkExe -if $(JLINK_INTERFACE) -device $(JLINK_DEVICE) -speed $(JLINK_SPEED)

else

flash:
	$(OPENOCD) -d2 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "reset halt" \
                 -c "flash write_image erase $(BUILD_DIR)/$(TARGET).elf" -c "verify_image $(BUILD_DIR)/$(TARGET).elf" -c "reset run" -c shutdown

halt:
	$(OPENOCD) -d0 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "halt" -c shutdown

reset:
	$(OPENOCD) -d0 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "reset" -c shutdown

openocd:
	$(OPENOCD) -d2 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "\$_TARGETNAME configure -rtos auto"


gdb: $(BUILD_DIR)/$(TARGET).elf
	$(GDB) -ex "target remote localhost:3333" -ex "monitor reset halt"

erase:
	$(OPENOCD) -d2 -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) -c init -c targets -c "halt" -c "stm32f1x mass_erase 0" -c shutdown

endif

#######################################
# clean up
#######################################
clean:
	-rm -fR .dep $(BUILD_DIR)
  
#######################################
# dependencies
#######################################
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

.PHONY: clean all

# *** EOF ***
