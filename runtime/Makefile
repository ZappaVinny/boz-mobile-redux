BUILD_DIR ?= build
X86_DIR := $(BUILD_DIR)/x86
HOST_DIR := $(BUILD_DIR)/host
HOST_CC ?= cc
TEST_CC := gcc -m32
X86_CC := gcc -m32
CLANG_FORMAT ?= clang-format
CFLAGS ?= -O2 -g
HOST_TEST_CFLAGS ?= -O0 -g
HOST_TEST_LDFLAGS ?=

UNICORN_LIB := $(X86_DIR)/unicorn/libunicorn.a
SDL2_LIB := $(X86_DIR)/sdl2-install/lib/libSDL2-2.0.so.0
LOADER_TARGET := $(X86_DIR)/codboz_s3e_loader
EXTRACT_TARGET := $(HOST_DIR)/codboz_apk_extract
TOOLCHAIN := $(abspath cmake/i686-linux.cmake)

PROJECT_CPPFLAGS := -D_GNU_SOURCE -Iinclude -Ithird_party/lzma
PROJECT_CFLAGS := -std=c11 -Wall -Wextra -Werror -Wno-format-truncation
X86_CFLAGS := $(CFLAGS) -falign-functions=16 -Ithird_party/unicorn/include

LOADER_SRC := \
  src/arm_emu.c \
  src/codboz_assets.c \
  src/codboz_frame_interpolation.c \
  src/main.c \
  src/s3e_config.c \
  src/s3e_audio.c \
  src/s3e_audio_unit.c \
  src/s3e_egl.c \
  src/s3e_file.c \
  src/s3e_gl.c \
  src/s3e_host.c \
  src/s3e_image.c \
  src/s3e_input.c \
  src/s3e_memory.c \
  src/device_id.c \
  src/s3e_runtime.c \
  src/s3e_timer.c \
  src/s3e_socket.c \
  src/mdns_wire.c \
  src/zeroconf_platform_posix.c \
  src/s3e_zeroconf.c
LOADER_OBJ := $(LOADER_SRC:%.c=$(X86_DIR)/%.o)

EXTRACT_SRC := tools/apk_extract/codboz_apk_extract.c third_party/lzma/LzmaDec.c
EXTRACT_OBJ := $(EXTRACT_SRC:%.c=$(HOST_DIR)/%.o)

HOST_TESTS := \
  $(HOST_DIR)/tests/s3e_socket_test \
  $(HOST_DIR)/tests/s3e_config_test \
  $(HOST_DIR)/tests/mdns_wire_test \
  $(HOST_DIR)/tests/s3e_zeroconf_test \
  $(HOST_DIR)/tests/s3e_timer_test \
  $(HOST_DIR)/tests/s3e_memory_test \
  $(HOST_DIR)/tests/device_id_test \
  $(HOST_DIR)/tests/s3e_audio_test \
  $(HOST_DIR)/tests/s3e_audio_unit_test \
  $(HOST_DIR)/tests/s3e_input_test
EMU_TEST := $(X86_DIR)/tests/arm_emu_test

FORMAT_FILES := $(shell find include src tests tools -type f \( -name '*.c' -o -name '*.h' \) | sort)

all: $(LOADER_TARGET) $(EXTRACT_TARGET) $(SDL2_LIB)

$(UNICORN_LIB):
	cmake -S third_party/unicorn -B $(X86_DIR)/unicorn -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) \
	  -DCMAKE_BUILD_TYPE=Release -DUNICORN_ARCH=arm -DUNICORN_BUILD_TESTS=OFF \
	  -DUNICORN_INSTALL=OFF -DBUILD_SHARED_LIBS=OFF
	cmake --build $(X86_DIR)/unicorn -j$$(nproc)

$(SDL2_LIB):
	cmake -S third_party/SDL2 -B $(X86_DIR)/sdl2 -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) \
	  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(abspath $(X86_DIR)/sdl2-install) \
	  -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF
	cmake --build $(X86_DIR)/sdl2 -j$$(nproc)
	cmake --install $(X86_DIR)/sdl2

$(LOADER_TARGET): $(LOADER_OBJ) $(UNICORN_LIB)
	$(X86_CC) -rdynamic -o $@ $(LOADER_OBJ) $(UNICORN_LIB) -ldl -pthread -lm

$(X86_DIR)/%.o: %.c | $(UNICORN_LIB)
	mkdir -p $(dir $@)
	$(X86_CC) $(PROJECT_CPPFLAGS) $(X86_CFLAGS) $(PROJECT_CFLAGS) -MMD -MP -c -o $@ $<

$(EXTRACT_TARGET): $(EXTRACT_OBJ)
	$(HOST_CC) -o $@ $^

$(HOST_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(HOST_CC) $(PROJECT_CPPFLAGS) $(CFLAGS) $(PROJECT_CFLAGS) -MMD -MP -c -o $@ $<

$(EMU_TEST): tests/arm_emu_test.c src/arm_emu.c include/arm_emu.h $(UNICORN_LIB)
	mkdir -p $(dir $@)
	$(X86_CC) $(PROJECT_CPPFLAGS) $(X86_CFLAGS) $(PROJECT_CFLAGS) -rdynamic -o $@ \
	  tests/arm_emu_test.c src/arm_emu.c $(UNICORN_LIB) -ldl -pthread -lm

$(HOST_DIR)/tests/s3e_socket_test: tests/s3e_socket_test.c src/s3e_socket.c src/s3e_config.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -pthread -o $@ $^ \
	  $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/s3e_config_test: tests/s3e_config_test.c src/s3e_config.c src/codboz_assets.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -o $@ $^ \
	  $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/mdns_wire_test: tests/mdns_wire_test.c src/mdns_wire.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -o $@ $^ \
	  $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/s3e_zeroconf_test: tests/s3e_zeroconf_test.c tests/zeroconf_platform_fake.c \
                                     src/s3e_zeroconf.c src/mdns_wire.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) -Itests $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -o $@ $^ \
	  $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/s3e_timer_test: tests/s3e_timer_test.c src/s3e_timer.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -pthread -o $@ $^ \
	  $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/s3e_memory_test: tests/s3e_memory_test.c src/s3e_memory.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -pthread -o $@ $^ \
	  $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/device_id_test: tests/device_id_test.c src/device_id.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -pthread -o $@ $^ \
	  $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/s3e_audio_test: tests/s3e_audio_test.c src/s3e_audio.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) -Ddlsym=s3e_audio_test_dlsym -Ddlclose=s3e_audio_test_dlclose \
	  $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -o $@ $^ $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/s3e_audio_unit_test: tests/s3e_audio_unit_test.c src/s3e_audio_unit.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -o $@ $^ \
	  $(HOST_TEST_LDFLAGS)

$(HOST_DIR)/tests/s3e_input_test: tests/s3e_input_test.c src/s3e_input.c tests/test_support.c
	mkdir -p $(dir $@)
	$(TEST_CC) $(PROJECT_CPPFLAGS) -Ddlsym=s3e_input_test_dlsym -Ddlclose=s3e_input_test_dlclose \
	  $(HOST_TEST_CFLAGS) $(PROJECT_CFLAGS) -o $@ $^ $(HOST_TEST_LDFLAGS)

test: $(HOST_TESTS) $(EMU_TEST)
	@set -e; for test in $(HOST_TESTS) $(EMU_TEST); do echo "== $$test"; "$$test"; done

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

clean:
	rm -rf $(BUILD_DIR)

-include $(LOADER_OBJ:.o=.d)
-include $(EXTRACT_OBJ:.o=.d)
.PHONY: all test format format-check clean
