# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
    ./test/FakeDbInterface.cpp \
    ./test/FakeLinkProber.cpp \
    ./test/FakeMuxPort.cpp \
    ./test/LinkManagerStateMachineTest.cpp \
    ./test/LinkProberTest.cpp \
    ./test/MuxManagerTest.cpp \
    ./test/LinkMgrdTestMain.cpp

OBJS_LINKMGRD_TEST += \
    ./test/FakeDbInterface.o \
    ./test/FakeLinkProber.o \
    ./test/FakeMuxPort.o \
    ./test/LinkManagerStateMachineTest.o \
    ./test/LinkProberTest.o \
    ./test/MuxManagerTest.o \
    ./test/LinkMgrdTestMain.o

CPP_DEPS += \
    ./test/FakeDbInterface.d \
    ./test/FakeLinkProber.d \
    ./test/FakeMuxPort.d \
    ./test/LinkManagerStateMachineTest.d \
    ./test/LinkProberTest.d \
    ./test/MuxManagerTest.d \
    ./test/LinkMgrdTestMain.d

# Each subdirectory must supply rules for building sources it contributes
test/%.o: test/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++17 -D__FILENAME__="$(subst src/,,$<)" -DBOOST_LOG_DYN_LINK $(INCLUDES) $(CPP_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


