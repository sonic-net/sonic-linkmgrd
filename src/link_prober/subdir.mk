# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
    ./src/link_prober/ActiveState.cpp \
    ./src/link_prober/PeerActiveState.cpp \
    ./src/link_prober/PeerUnknownState.cpp \
    ./src/link_prober/PeerWaitState.cpp \
    ./src/link_prober/IcmpPayload.cpp \
    ./src/link_prober/LinkProber.cpp \
    ./src/link_prober/LinkProberState.cpp \
    ./src/link_prober/LinkProberStateMachineBase.cpp \
    ./src/link_prober/LinkProberStateMachineActiveStandby.cpp \
    ./src/link_prober/LinkProberStateMachineActiveActive.cpp \
    ./src/link_prober/LinkProberSessionStateMachine.cpp \
    ./src/link_prober/SelfInitState.cpp \
    ./src/link_prober/SelfUpState.cpp \
    ./src/link_prober/SelfDownState.cpp \
    ./src/link_prober/PeerInitState.cpp \
    ./src/link_prober/PeerUpState.cpp \
    ./src/link_prober/PeerDownState.cpp \
    ./src/link_prober/StandbyState.cpp \
    ./src/link_prober/UnknownState.cpp \
    ./src/link_prober/WaitState.cpp 

OBJS += \
    ./src/link_prober/ActiveState.o \
    ./src/link_prober/PeerActiveState.o \
    ./src/link_prober/PeerUnknownState.o \
    ./src/link_prober/PeerWaitState.o \
    ./src/link_prober/IcmpPayload.o \
    ./src/link_prober/LinkProber.o \
    ./src/link_prober/LinkProberState.o \
    ./src/link_prober/LinkProberStateMachineBase.o \
    ./src/link_prober/LinkProberStateMachineActiveStandby.o \
    ./src/link_prober/LinkProberStateMachineActiveActive.o \
    ./src/link_prober/LinkProberSessionStateMachine.o \
    ./src/link_prober/SelfInitState.o \
    ./src/link_prober/SelfUpState.o \
    ./src/link_prober/SelfDownState.o \
    ./src/link_prober/PeerInitState.o \
    ./src/link_prober/PeerUpState.o \
    ./src/link_prober/PeerDownState.o \
    ./src/link_prober/StandbyState.o \
    ./src/link_prober/UnknownState.o \
    ./src/link_prober/WaitState.o 

CPP_DEPS += \
    ./src/link_prober/ActiveState.d \
    ./src/link_prober/PeerActiveState.d \
    ./src/link_prober/PeerUnknownState.d \
    ./src/link_prober/PeerWaitState.d \
    ./src/link_prober/IcmpPayload.d \
    ./src/link_prober/LinkProber.d \
    ./src/link_prober/LinkProberState.d \
    ./src/link_prober/LinkProberStateMachineBase.d \
    ./src/link_prober/LinkProberStateMachineActiveStandby.d \
    ./src/link_prober/LinkProberStateMachineActiveActive.d \
    ./src/link_prober/LinkProberSessionStateMachine.d \
    ./src/link_prober/SelfInitState.d \
    ./src/link_prober/SelfUpState.d \
    ./src/link_prober/SelfDownState.d \
    ./src/link_prober/PeerInitState.d \
    ./src/link_prober/PeerUpState.d \
    ./src/link_prober/PeerDownState.d \
    ./src/link_prober/StandbyState.d \
    ./src/link_prober/UnknownState.d \
    ./src/link_prober/WaitState.d 


# Each subdirectory must supply rules for building sources it contributes
src/link_prober/%.o: src/link_prober/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -std=c++17 -D__FILENAME__="$(subst src/,,$<)" $(BOOST_MACROS) $(INCLUDES) $(CPP_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


