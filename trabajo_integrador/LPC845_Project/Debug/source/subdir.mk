################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/LPC845_Project.c \
../source/mtb.c \
../source/semihost_hardfault.c 

C_DEPS += \
./source/LPC845_Project.d \
./source/mtb.d \
./source/semihost_hardfault.d 

OBJS += \
./source/LPC845_Project.o \
./source/mtb.o \
./source/semihost_hardfault.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_LPC845M301JBD48 -DCPU_LPC845M301JBD48_cm0plus -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\LPC845_Project\board" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\LPC845_Project\source" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\LPC845_Project\component\uart" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\LPC845_Project\drivers" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\LPC845_Project\CMSIS" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\LPC845_Project\device" -I"C:\Users\estudiante\Documents\GitHub\curso-lse\trabajo_integrador\LPC845_Project\utilities" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/LPC845_Project.d ./source/LPC845_Project.o ./source/mtb.d ./source/mtb.o ./source/semihost_hardfault.d ./source/semihost_hardfault.o

.PHONY: clean-source

