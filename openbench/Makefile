# Default generator, can be overridden by setting CMAKE_GENERATOR.
CMAKE_GENERATOR ?= "Unix Makefiles"

# Default number of parallel jobs for the build.
JOBS ?= 6

# CMake build directory.
BUILD_DIR = build

# CMake options.
BASE_CMAKE_OPTIONS = -DCMAKE_BUILD_TYPE=Release -DDEVELOPMENT=ON -DTUNING=ON -DINCLUDE_TRACING_MODULE=OFF -DOPENBENCH_COMPLIANCE_MODE=ON
CMAKE_OPTIONS = $(BASE_CMAKE_OPTIONS) $(CMAKE_ARGS)

# Set default target; can be overridden.
TARGET ?= illumina_cli_base
BINARY ?= illumina_base

# Detect CPU capabilities and set the best target and binary name
# Prioritize BMI2 architecture check over AVX2
ifeq ($(shell grep -m1 -o bmi2 /proc/cpuinfo 2>/dev/null),bmi2)
	TARGET := illumina_cli_bmi2
	BINARY := illumina_bmi2
else ifeq ($(shell grep -m1 -o avx2 /proc/cpuinfo 2>/dev/null),avx2)
	TARGET := illumina_cli_avx2
	BINARY := illumina_avx2
endif

# Detect if the environment is Windows and append the .exe extension if so.
ifeq ($(OS),Windows_NT)
	EXEC_EXT := .exe
else
	EXEC_EXT :=
endif

EVALFILE ?=

# Resolve EVALFILE to its absolute path.
ifneq ($(EVALFILE),)
    ABSOLUTE_EVALFILE := $(shell realpath $(EVALFILE))
endif

# Set the C and C++ compilers, either from environment variables or defaults.
# Prioritize Clang over GCC if available.
CC  ?= $(shell command -v clang || echo gcc)
CXX ?= $(shell command -v clang++ || echo g++)

# Default target to configure, build, and move the best binary.
all: clean configure build move

# Run CMake to configure the project.
configure:
	cmake -S .. -B $(BUILD_DIR) -G $(CMAKE_GENERATOR) $(CMAKE_OPTIONS) \
		  -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) \
		  $(if $(ABSOLUTE_EVALFILE),-DNNUE_PATH=$(ABSOLUTE_EVALFILE))

# Build the selected target.
build: configure
	@if [ -f $(BUILD_DIR)/Makefile ]; then \
	    $(MAKE) -C $(BUILD_DIR) -j$(JOBS) $(TARGET); \
	else \
	    echo "Error: No Makefile found in $(BUILD_DIR). Run 'make configure' first."; \
	    exit 1; \
	fi

# Move the built binary to the current directory.
move: build
	@if [ -f $(BUILD_DIR)/cli/$(BINARY)$(EXEC_EXT) ]; then \
	    mv $(BUILD_DIR)/cli/$(BINARY)$(EXEC_EXT) ./; \
	else \
	    echo "Error: Binary $(BINARY)$(EXEC_EXT) not found in $(BUILD_DIR)/cli."; \
	    exit 1; \
	fi

	@if [ ! -z "$(EXE)" ]; then \
	    mv $(BINARY)$(EXEC_EXT) $(EXE)$(EXEC_EXT); \
	fi

# Clean the build directory and binaries.
clean:
	rm -rf $(BUILD_DIR)
	rm -f illumina_base illumina_bmi2 illumina_avx2
