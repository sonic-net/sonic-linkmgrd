# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
    ./test/FakeDbInterface.cpp \
    ./test/FakeLinkProber.cpp \
    ./test/FakeMuxPort.cpp \
    ./test/LinkManagerStateMachineTest.cpp \
    ./test/LinkManagerStateMachineActiveActiveTest.cpp \
    ./test/LinkProberTest.cpp \
    ./test/MuxManagerTest.cpp \
    ./test/MockLinkManagerStateMachine.cpp \
    ./test/MockLinkProberTest.cpp \
    ./test/LinkMgrdTestMain.cpp \
    ./test/MuxLoggerTest.cpp \
    ./test/MockMuxPort.cpp \
    ./test/MockLinkProberStateMachineTest.cpp

OBJS_LINKMGRD_TEST += \
    ./test/FakeDbInterface.o \
    ./test/FakeLinkProber.o \
    ./test/FakeMuxPort.o \
    ./test/LinkManagerStateMachineTest.o \
    ./test/LinkManagerStateMachineActiveActiveTest.o \
    ./test/LinkProberTest.o \
    ./test/MuxManagerTest.o \
    ./test/MockLinkManagerStateMachine.o \
    ./test/MockLinkProberTest.o \
    ./test/LinkMgrdTestMain.o \
    ./test/MuxLoggerTest.o \
    ./test/MockMuxPort.o \
    ./test/MockLinkProberStateMachineTest.o

CPP_DEPS += \
    ./test/FakeDbInterface.d \
    ./test/FakeLinkProber.d \
    ./test/FakeMuxPort.d \
    ./test/LinkManagerStateMachineTest.d \
    ./test/LinkManagerStateMachineActiveActiveTest.d \
    ./test/LinkProberTest.d \
    ./test/MuxManagerTest.d \
    ./test/MockLinkManagerStateMachine.d \
    ./test/MockLinkProberTest.d \
    ./test/LinkMgrdTestMain.d \
    ./test/MuxLoggerTest.d \
    ./test/MockMuxPort.d \
    ./test/MockLinkProberStateMachineTest.d

# Each subdirectory must supply rules for building sources it contributes
test/%.o: test/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++17 -D__FILENAME__="$(subst src/,,$<)" -DBOOST_LOG_DYN_LINK $(INCLUDES) $(CPP_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


