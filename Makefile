-include ../makefile.init

GCOVR := gcovr
FIND := find
MAKE := make
RM := rm -rf
LINKMGRD_TARGET := linkmgrd
LINKMGRD_TEST_TARGET := linkmgrd-test
CP := cp
MKDIR := mkdir
CC := g++
MV := mv
BOOST_MACROS = -DBOOST_LOG_USE_NATIVE_SYSLOG -DBOOST_LOG_DYN_LINK
GCOV_FLAGS := -fprofile-arcs -ftest-coverage
TOPDIR := $(dir $(firstword $(MAKEFILE_LIST)))
MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS := $(subst -j,,$(JOB_FLAG))

release-targets: CPP_FLAGS := -O3 -Wall -c -fmessage-length=0 -fPIC -flto
test-targets: CPP_FLAGS = -O0 -Wall -c -fmessage-length=0 -fPIC $(GCOV_FLAGS)

override INCLUDES += -I"$(TOPDIR)/src" -I"/usr/include/libnl3/"

# All of the sources participating in the build are defined here
-include sources.mk
-include test/subdir.mk
-include src/mux_state/subdir.mk
-include src/link_state/subdir.mk
-include src/link_prober/subdir.mk
-include src/link_manager/subdir.mk
-include src/common/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: sonic-linkmgrd

# Tool invocations
release-targets: $(OBJS) $(USER_OBJS) $(OBJS_LINKMGRD)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	$(CC) -pthread -o "$(LINKMGRD_TARGET)" $(OBJS) $(OBJS_LINKMGRD) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

sonic-linkmgrd: clean-targets
	$(MAKE) -j $(JOBS) release-targets

# test Target
test-targets: $(OBJS) $(USER_OBJS) $(OBJS_LINKMGRD_TEST)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	$(CC) -pthread -fprofile-generate -lgcov -o "$(LINKMGRD_TEST_TARGET)" $(OBJS) $(OBJS_LINKMGRD_TEST) $(USER_OBJS) $(LIBS) $(LIBS_TEST)
	@echo 'Executing test target: $@'
	LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libasan.so.5 ./$(LINKMGRD_TEST_TARGET)
	$(GCOVR) -r ./ --html --html-details -o $(LINKMGRD_TEST_TARGET)-result.html
	$(GCOVR) -r ./ --xml-pretty -o $(LINKMGRD_TEST_TARGET)-result.xml
	@echo 'Finished building target: $@'
	@echo ' '

test: clean-targets
	$(MAKE) -j $(JOBS) test-targets
	
install:
	$(MKDIR) -p $(DESTDIR)/usr/sbin
	$(MV) $(LINKMGRD_TARGET) $(DESTDIR)/usr/sbin
	$(RM) $(CC_DEPS) $(C++_DEPS) $(EXECUTABLES) $(C_UPPER_DEPS) $(CXX_DEPS) $(OBJS) $(CPP_DEPS) $(C_DEPS) \
		$(LINKMGRD_TARGET) $(LINKMGRD_TEST_TARGET) $(OBJS_LINKMGRD) $(OBJS_LINKMGRD_TEST)

deinstall:
	$(RM) $(DESTDIR)/usr/sbin/$(LINKMGRD_TARGET)
	$(RM) -rf $(DESTDIR)/usr/sbin

clean-targets:
	$(RM) $(CC_DEPS) $(C++_DEPS) $(EXECUTABLES) $(C_UPPER_DEPS) $(CXX_DEPS) $(OBJS) $(CPP_DEPS) $(C_DEPS) \
		$(OBJS_LINKMGRD) $(OBJS_LINKMGRD_TEST)

clean: clean-targets
	$(RM) $(LINKMGRD_TARGET) $(LINKMGRD_TEST_TARGET) *.html linkmgrd-test-result.xml
	$(FIND) . -name *.gcda -exec rm -f {} \;
	$(FIND) . -name *.gcno -exec rm -f {} \;
	$(FIND) . -name *.gcov -exec rm -f {} \;
	@echo ' '

.PHONY: all clean dependents

-include ../makefile.targets
