################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/interrupt_example.c \
../src/platform.c \
../src/zynq-converter-controller.c 

OBJS += \
./src/interrupt_example.o \
./src/platform.o \
./src/zynq-converter-controller.o 

C_DEPS += \
./src/interrupt_example.d \
./src/platform.d \
./src/zynq-converter-controller.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../zynq-converter-project_bsp/ps7_cortexa9_0/include -I"D:\XilinxWokrspace\zybo_platform" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


