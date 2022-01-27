################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/controller.c \
../src/interrupts_buttons.c \
../src/led_drivers.c \
../src/pid.c \
../src/platform.c \
../src/uart_comms.c \
../src/zynq-converter-controller.c 

OBJS += \
./src/controller.o \
./src/interrupts_buttons.o \
./src/led_drivers.o \
./src/pid.o \
./src/platform.o \
./src/uart_comms.o \
./src/zynq-converter-controller.o 

C_DEPS += \
./src/controller.d \
./src/interrupts_buttons.d \
./src/led_drivers.d \
./src/pid.d \
./src/platform.d \
./src/uart_comms.d \
./src/zynq-converter-controller.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I"D:\XilinxWokrspace\zybo_platform" -I../../zynq-converter-project_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


