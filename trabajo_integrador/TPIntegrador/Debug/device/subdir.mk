################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../device/system_LPC845.c 

C_DEPS += \
./device/system_LPC845.d 

OBJS += \
./device/system_LPC845.o 


# Each subdirectory must supply rules for building sources it contributes
device/%.o: ../device/%.c device/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_LPC845M301JBD48 -DCPU_LPC845M301JBD48_cm0plus -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\TPIntegrador\board" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\TPIntegrador\source" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\TPIntegrador\component\uart" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\TPIntegrador\drivers" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\TPIntegrador\CMSIS" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\TPIntegrador\device" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\TPIntegrador\utilities" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\freertos\inc" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-device

clean-device:
	-$(RM) ./device/system_LPC845.d ./device/system_LPC845.o

.PHONY: clean-device

