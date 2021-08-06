# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
    ./src/mux_state/ActiveState.cpp \
    ./src/mux_state/MuxState.cpp \
    ./src/mux_state/MuxStateMachine.cpp \
    ./src/mux_state/StandbyState.cpp \
    ./src/mux_state/UnknownState.cpp \
    ./src/mux_state/ErrorState.cpp \
    ./src/mux_state/WaitState.cpp 

OBJS += \
    ./src/mux_state/ActiveState.o \
    ./src/mux_state/MuxState.o \
    ./src/mux_state/MuxStateMachine.o \
    ./src/mux_state/StandbyState.o \
    ./src/mux_state/UnknownState.o \
    ./src/mux_state/ErrorState.o \
    ./src/mux_state/WaitState.o 

CPP_DEPS += \
    ./src/mux_state/ActiveState.d \
    ./src/mux_state/MuxState.d \
    ./src/mux_state/MuxStateMachine.d \
    ./src/mux_state/StandbyState.d \
    ./src/mux_state/UnknownState.d \
    ./src/mux_state/ErrorState.d \
    ./src/mux_state/WaitState.d 


# Each subdirectory must supply rules for building sources it contributes
src/mux_state/%.o: src/mux_state/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -std=c++17 -D__FILENAME__="$(subst src/,,$<)" $(BOOST_MACROS) $(INCLUDES) $(CPP_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


