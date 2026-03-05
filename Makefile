CC = gcc
CFLAGS_COMMON = -Wall -Wextra -std=c11 -I./include
LDFLAGS_COMMON =

# Build Profiles
CFLAGS_DEBUG = $(CFLAGS_COMMON) -g -O0
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O3 -DNDEBUG
CFLAGS_PERF = $(CFLAGS_RELEASE) -march=native -fno-plt -static
CFLAGS_VERIFY = $(CFLAGS_COMMON) -O2 -static

# Directories
SRC_DIR = src
INC_DIR = include
TOOLS_DIR = tools
TESTS_DIR = tests
BIN_DIR = bin
BUILD_DIR = build

# Source files (Engine)
ENGINE_SRCS = $(wildcard $(SRC_DIR)/*.c)
ENGINE_OBJS_PERF = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/perf/%.o, $(ENGINE_SRCS))
ENGINE_OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/debug/%.o, $(ENGINE_SRCS))

# Default profile is perf for benchmarking as per TAB_PROTOCOL
all: perf

# --- Perf Build ---
perf: $(BIN_DIR)/hebs_bench_runner

$(BIN_DIR)/hebs_bench_runner: $(TOOLS_DIR)/hebs_bench_runner.c $(ENGINE_OBJS_PERF) | $(BIN_DIR)
	$(CC) $(CFLAGS_PERF) $^ -o $@ $(LDFLAGS_COMMON)

$(BUILD_DIR)/perf/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/perf
	$(CC) $(CFLAGS_PERF) -c $< -o $@

# --- Debug Build ---
debug: $(BIN_DIR)/hebs_bench_runner_debug

$(BIN_DIR)/hebs_bench_runner_debug: $(TOOLS_DIR)/hebs_bench_runner.c $(ENGINE_OBJS_DEBUG) | $(BIN_DIR)
	$(CC) $(CFLAGS_DEBUG) $^ -o $@ $(LDFLAGS_COMMON)

$(BUILD_DIR)/debug/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/debug
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# --- Verification & Tests ---
verify: $(BIN_DIR)/hebs_verify_runner
	@echo "Running Verification Suite..."
	# $(BIN_DIR)/hebs_verify_runner

$(BIN_DIR)/hebs_verify_runner: $(TESTS_DIR)/verify_runner.c $(ENGINE_OBJS_DEBUG) | $(BIN_DIR)
	$(CC) $(CFLAGS_VERIFY) $^ -o $@ $(LDFLAGS_COMMON)

# --- Directory Creation ---
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/perf:
	mkdir -p $(BUILD_DIR)/perf

$(BUILD_DIR)/debug:
	mkdir -p $(BUILD_DIR)/debug

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/*

.PHONY: all perf debug verify clean
