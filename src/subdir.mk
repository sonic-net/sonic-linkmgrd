# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
    ./src/DbInterface.cpp \
    ./src/LinkMgrdMain.cpp \
    ./src/MuxManager.cpp \
    ./src/MuxPort.cpp \
    ./src/NetMsgInterface.cpp

OBJS += \
    ./src/DbInterface.o \
    ./src/MuxManager.o \
    ./src/MuxPort.o \
    ./src/NetMsgInterface.o

OBJS_LINKMGRD += \
    ./src/LinkMgrdMain.o \

CPP_DEPS += \
    ./src/DbInterface.d \
    ./src/LinkMgrdMain.d \
    ./src/MuxManager.d \
    ./src/MuxPort.d \
    ./src/NetMsgInterface.d


# Each subdirectory must supply rules for building sources it contributes
src/%.o: src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -std=c++17 -D__FILENAME__="$(subst src/,,$<)" $(BOOST_MACROS) $(INCLUDES) $(CPP_FLAGS) -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


