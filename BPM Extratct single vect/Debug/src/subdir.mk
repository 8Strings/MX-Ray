################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/BPM\ Extratct\ single\ vect.cpp 

OBJS += \
./src/BPM\ Extratct\ single\ vect.o 

CPP_DEPS += \
./src/BPM\ Extratct\ single\ vect.d 


# Each subdirectory must supply rules for building sources it contributes
src/BPM\ Extratct\ single\ vect.o: ../src/BPM\ Extratct\ single\ vect.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/BPM Extratct single vect.d" -MT"src/BPM\ Extratct\ single\ vect.d" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


