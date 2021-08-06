# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
    ./src/link_prober/ActiveState.cpp \
    ./src/link_prober/IcmpPayload.cpp \
    ./src/link_prober/LinkProber.cpp \
    ./src/link_prober/LinkProberState.cpp \
    ./src/link_prober/LinkProberStateMachine.cpp \
    ./src/link_prober/StandbyState.cpp \
    ./src/link_prober/UnknownState.cpp \
    ./src/link_prober/WaitState.cpp 

OBJS += \
    ./src/link_prober/ActiveState.o \
    ./src/link_prober/IcmpPayload.o \
    ./src/link_prober/LinkProber.o \
    ./src/link_prober/LinkProberState.o \
    ./src/link_prober/LinkProberStateMachine.o \
    ./src/link_prober/StandbyState.o \
    ./src/link_prober/UnknownState.o \
    ./src/link_prober/WaitState.o 

CPP_DEPS += \
    ./src/link_prober/ActiveState.d \
    ./src/link_prober/IcmpPayload.d \
    ./src/link_prober/LinkProber.d \
    ./src/link_prober/LinkProberState.d \
    ./src/link_prober/LinkProberStateMachine.d \
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


