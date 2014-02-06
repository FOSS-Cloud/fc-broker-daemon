################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../config.cpp \
../basePolicy.cpp \
../cronConfiguration.cpp \
../evenlyPolicy.cpp \
../evenlyPolicyInterval.cpp \
../ldapTools.cpp \
../main.cpp \
../networkRange.cpp \
../node.cpp \
../signalHandler.cpp \
../virtTools.cpp \
../vm.cpp \
../vmFactory.cpp \
../vmPool.cpp 

OBJS += \
./config.o \
./basePolicy.o \
./cronConfiguration.o \
./evenlyPolicy.o \
./evenlyPolicyInterval.o \
./ldapTools.o \
./main.o \
./networkRange.o \
./node.o \
./signalHandler.o \
./virtTools.o \
./vm.o \
./vmFactory.o \
./vmPool.o 

CPP_DEPS += \
./config.d \
./basePolicy.d \
./cronConfiguration.d \
./evenlyPolicy.d \
./evenlyPolicyInterval.d \
./ldapTools.d \
./main.d \
./networkRange.d \
./node.d \
./signalHandler.d \
./virtTools.d \
./vm.d \
./vmFactory.d \
./vmPool.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -I/usr/include -O3 -Wall -c -fmessage-length=0  -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


