# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
	./src/link_manager/LinkManagerStateMachineBase.cpp \
    ./src/link_manager/LinkManagerStateMachineActiveStandby.cpp \
    ./src/link_manager/LinkManagerStateMachineActiveActive.cpp \

OBJS += \
	./src/link_manager/LinkManagerStateMachineBase.o \
    ./src/link_manager/LinkManagerStateMachineActiveStandby.o \
    ./src/link_manager/LinkManagerStateMachineActiveActive.o \

CPP_DEPS += \
	./src/link_manager/LinkManagerStateMachineBase.d \
    ./src/link_manager/LinkManagerStateMachineActiveStandby.d \
    ./src/link_manager/LinkManagerStateMachineActiveActive.d \

# Each subdirectory must supply rules for building sources it contributes
src/link_manager/%.o: src/link_manager/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -std=c++17 -D__FILENAME__="$(subst src/,,$<)" $(BOOST_MACROS) $(INCLUDES) $(CPP_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


