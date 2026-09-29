# Host unit tests and firmware checks. The flashing build is the CCS project.
#
#   make test        host unit tests for src/safety.c (gcc, Unity)
#   make arm-check   compile the firmware modules for Cortex-M4, warnings as errors
#   make cppcheck    static analysis of the same modules
#   make check       all three (what CI runs)

CC        = gcc
ARM_CC    = arm-none-eabi-gcc
CPPCHECK  = cppcheck
BUILD     := build

# Firmware modules written for this project. TI, Adafruit and course files
# are compiled by CCS but not checked here.
FW_SRC := src/main.c src/balance.c src/kalman.c src/motor.c src/mpu6050.c \
          src/encoder.c src/clock.c src/pinmux.c src/cart_email.c src/safety.c

# ci/sdk_stubs stands in for the CC3200 SDK headers. aws_config.h is not in
# git, so the checks use the example copy under build/cfg.
FW_INC  := -Isrc -Isrc/common -Ici/sdk_stubs -I$(BUILD)/cfg
FW_DEFS := -Dccs -Dcc3200

ARM_CFLAGS := -mcpu=cortex-m4 -mthumb -std=c99 -ffreestanding -O2 \
              -Wall -Wextra -Werror $(ARM_EXTRA_CFLAGS)

HOST_CFLAGS := -std=c99 -O0 -g -Wall -Wextra -Werror \
               -fsanitize=address,undefined -fno-sanitize-recover=all \
               -fno-omit-frame-pointer

TEST_SRC := tests/test_safety.c tests/fake_hal.c tests/ref_main_loop.c \
            tests/unity/unity.c src/safety.c
TEST_HDR := tests/fake_hal.h tests/ref_main_loop.h src/safety.h src/hw_config.h

ARM_OBJ := $(patsubst src/%.c,$(BUILD)/arm/%.o,$(FW_SRC))

.PHONY: all check test arm-check cppcheck clean

all: check
check: test arm-check cppcheck

test: $(BUILD)/test_safety
	./$(BUILD)/test_safety

$(BUILD)/test_safety: $(TEST_SRC) $(TEST_HDR)
	@mkdir -p $(@D)
	$(CC) $(HOST_CFLAGS) -Isrc -Ici/sdk_stubs -Itests -Itests/unity \
	    -o $@ $(TEST_SRC)

$(BUILD)/cfg/aws_config.h: src/aws_config.h.example
	@mkdir -p $(@D)
	cp $< $@

arm-check: $(ARM_OBJ)
	@echo "arm-check: $(words $(ARM_OBJ)) modules compiled for Cortex-M4, no warnings"

$(BUILD)/arm/%.o: src/%.c $(BUILD)/cfg/aws_config.h
	@mkdir -p $(@D)
	$(ARM_CC) $(ARM_CFLAGS) $(FW_DEFS) $(FW_INC) -MMD -MP -c $< -o $@

-include $(ARM_OBJ:.o=.d)

cppcheck: $(BUILD)/cfg/aws_config.h
	$(CPPCHECK) --std=c99 --enable=warning,style,performance,portability \
	    --error-exitcode=1 --inline-suppr --quiet \
	    $(FW_DEFS) $(FW_INC) $(FW_SRC)

clean:
	rm -rf $(BUILD)
