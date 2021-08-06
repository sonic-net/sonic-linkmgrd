# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
    ./src/link_state/DownState.cpp \
    ./src/link_state/LinkState.cpp \
    ./src/link_state/LinkStateMachine.cpp \
    ./src/link_state/UpState.cpp 

OBJS += \
    ./src/link_state/DownState.o \
    ./src/link_state/LinkState.o \
    ./src/link_state/LinkStateMachine.o \
    ./src/link_state/UpState.o 

CPP_DEPS += \
    ./src/link_state/DownState.d \
    ./src/link_state/LinkState.d \
    ./src/link_state/LinkStateMachine.d \
    ./src/link_state/UpState.d 


# Each subdirectory must supply rules for building sources it contributes
src/link_state/%.o: src/link_state/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -std=c++17 -D__FILENAME__="$(subst src/,,$<)" $(BOOST_MACROS) $(INCLUDES) $(CPP_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


