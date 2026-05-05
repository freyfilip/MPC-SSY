################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/241161/dnes3/Zigbee_TempMeas_Server_Router/Projects/Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.c 

OBJS += \
./Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.o 

C_DEPS += \
./Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.d 


# Each subdirectory must supply rules for building sources it contributes
Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.o: D:/241161/dnes3/Zigbee_TempMeas_Server_Router/Projects/Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.c Common/WPAN/Modules/SerialCmdInterpreter/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DSTM32 -DSTM32WBA -DSTM32WBA55xx -DSTM32WBA55CGUx -DUSE_HAL_DRIVER -DUSE_FULL_ASSERT -DUSE_FULL_LL_DRIVER -DMAC -DMAC_LAYER -DAPPLICATION_USE_BAREMETAL -DAPPLICATION_USE_NUCLEO -DAPPLICATION_USE_HW_AES -DAPPLICATION_USE_HW_RNG -c -I../../Core/Inc -I../../STM32_WPAN/App -I../../STM32_WPAN/Target -I../../System/Config/CRC_Ctrl -I../../System/Config/Debug_GPIO -I../../System/Config -I../../System/Config/Log -I../../System/Config/LowPower -I../../System/Interfaces -I../../Projects/Common/WPAN/Interfaces -I../../Projects/Common/WPAN/Modules -I../../Projects/Common/WPAN/Modules/BasicAES -I../../Projects/Common/WPAN/Modules/Flash -I../../Projects/Common/WPAN/Modules/Log -I../../Projects/Common/WPAN/Modules/MemoryManager -I../../Projects/Common/WPAN/Modules/RTDebug -I../../Projects/Common/WPAN/Modules/SerialCmdInterpreter -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32WBAxx/Include -I../../Drivers/STM32WBAxx_HAL_Driver/Inc -I../../Drivers/BSP/Components/Common -I../../Drivers/BSP/STM32WBAxx_Nucleo -I../../Middlewares/ST/STM32_WPAN -I../../Middlewares/ST/STM32_WPAN/zigbee/include -I../../Middlewares/ST/STM32_WPAN/zigbee/stack/include -I../../Middlewares/ST/STM32_WPAN/zigbee/stack/port/stm32wba -I../../Middlewares/ST/STM32_WPAN/zigbee/stack/port/mac -I../../Middlewares/ST/STM32_WPAN/mac_802_15_4/core/inc -I../../Middlewares/ST/STM32_WPAN/link_layer/ll_sys/inc -I../../Middlewares/ST/STM32_WPAN/link_layer/ll_cmd_lib/config/ieee_15_4_basic -I../../Middlewares/ST/STM32_WPAN/link_layer/ll_cmd_lib/inc -I../../Middlewares/ST/STM32_WPAN/link_layer/ll_cmd_lib/inc/ot_inc -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Middlewares/ST/STM32_WPAN/interface -I../../Utilities/conf -I../../Utilities/lpm/tiny_lpm -I../../Utilities/misc -I../../Utilities/sequencer -I../../Utilities/tim_serv -I../../Utilities/trace/adv_trace -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Common-2f-WPAN-2f-Modules-2f-SerialCmdInterpreter

clean-Common-2f-WPAN-2f-Modules-2f-SerialCmdInterpreter:
	-$(RM) ./Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.cyclo ./Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.d ./Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.o ./Common/WPAN/Modules/SerialCmdInterpreter/serial_cmd_interpreter.su

.PHONY: clean-Common-2f-WPAN-2f-Modules-2f-SerialCmdInterpreter

