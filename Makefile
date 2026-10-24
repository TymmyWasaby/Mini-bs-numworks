Q ?= @
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
BUILD_DIR = target
NWLINK = npx --yes -- nwlink@0.0.19

CPPFLAGS = -Isrc -std=c++11 -fno-exceptions -fdata-sections -ffunction-sections
CPPFLAGS += -Os -Wall -Wextra
CPPFLAGS += $(shell $(NWLINK) eadk-cflags-device)
CPPFLAGS += -flto -fno-fat-lto-objects -fwhole-program
LDFLAGS = -Wl,--relocatable -nostartfiles --specs=nano.specs
LDFLAGS += -Wl,-e,main -Wl,-u,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -flinker-output=nolto-rel

.PHONY: all build clean
all: build
build: $(BUILD_DIR)/mini-bs.nwa

$(BUILD_DIR)/mini-bs.nwa: $(BUILD_DIR)/game.o $(BUILD_DIR)/icon.o
    @echo "Linking $@"
    $(Q) $(CC) $(CPPFLAGS) $(LDFLAGS) $^ -lm -o $@

$(BUILD_DIR)/game.o: src/game.cpp src/ion.h | $(BUILD_DIR)
    @echo "Compiling $<"
    $(Q) $(CXX) $(CPPFLAGS) -fno-rtti -c $< -o $@

$(BUILD_DIR)/icon.o: src/icon.png | $(BUILD_DIR)
    @echo "Embedding icon"
    $(Q) $(NWLINK) png-icon-o $< $@

$(BUILD_DIR):
    $(Q) mkdir -p $(BUILD_DIR)

clean:
    rm -rf $(BUILD_DIR)
