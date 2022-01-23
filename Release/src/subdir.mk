################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/controller.c \
../src/interrupt_example.c \
../src/interrupts_buttons.c \
../src/pid.c \
../src/platform.c \
../src/zynq-converter-controller.c 

OBJS += \
./src/controller.o \
./src/interrupt_example.o \
./src/interrupts_buttons.o \
./src/pid.o \
./src/platform.o \
./src/zynq-converter-controller.o 

C_DEPS += \
./src/controller.d \
./src/interrupt_example.d \
./src/interrupts_buttons.d \
./src/pid.d \
./src/platform.d \
./src/zynq-converter-controller.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O2 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../zynq-converter-project_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


