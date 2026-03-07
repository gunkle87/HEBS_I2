# ==============================================================================
# HEBS_CLEAN MASTER MAKEFILE - REVISION_STRUCTURE_V01 (Windows Native)
# ==============================================================================

# 1. PATH DEFINITIONS (Windows-friendly backslashes for shell commands)
CORE_DIR       = core
INCLUDES_DIR   = include
BENCH_DIR      = benchmarks
BENCH_FILES    = benchmarks\benches
RESULT_DIR     = benchmarks\results
TEST_DIR       = tests
TEST_RESULTS   = tests\results
BUILD_DIR      = build

# 2. COMPILER SETTINGS
CC             = gcc
# Note: GCC still prefers forward slashes for include paths
CFLAGS         = -Iinclude -O3 -march=native -Wall -Wextra -pthread
LDFLAGS        = -pthread

# 3. TARGETS
CORE_SRC       = core/engine.c core/loader.c core/primitives.c core/state_manager.c
RUNNER_SRC     = benchmarks/runner.c
METRIC_SRC     = benchmarks/metric_calculator.c
TEST_SRC       = tests/test_runner.c

# 4. BUILD RULES
all: setup hebs_cli hebs_metrics hebs_test

# Setup using Windows 'cmd' syntax
setup:
	@if not exist $(CORE_DIR) mkdir $(CORE_DIR)
	@if not exist $(INCLUDES_DIR) mkdir $(INCLUDES_DIR)
	@if not exist $(BENCH_DIR) mkdir $(BENCH_DIR)
	@if not exist $(BENCH_FILES) mkdir $(BENCH_FILES)
	@if not exist $(RESULT_DIR) mkdir $(RESULT_DIR)
	@if not exist $(TEST_DIR) mkdir $(TEST_DIR)
	@if not exist $(TEST_RESULTS) mkdir $(TEST_RESULTS)
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@echo HEBS_CLEAN: Directory structure initialized.

# Compile the Benchmark Runner
hebs_cli:
	$(CC) $(CFLAGS) $(CORE_SRC) $(RUNNER_SRC) -o $(BUILD_DIR)/hebs_cli $(LDFLAGS)

# Compile the Metric Calculator
hebs_metrics:
	$(CC) $(CFLAGS) $(METRIC_SRC) -o $(BUILD_DIR)/hebs_metrics $(LDFLAGS)

# Compile the Test Suite
hebs_test:
	$(CC) $(CFLAGS) $(CORE_SRC) $(TEST_SRC) -o $(BUILD_DIR)/hebs_test $(LDFLAGS)

# 5. EXECUTION TARGETS
verify:
	.\$(BUILD_DIR)\hebs_test

clean:
	@if exist $(BUILD_DIR) rd /s /q $(BUILD_DIR)
	@echo HEBS_CLEAN: Build artifacts removed.

.PHONY: all setup clean verify hebs_cli hebs_metrics hebs_test
