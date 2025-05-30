#===============================================================================
# Version Checks (GCC >= 14.2.0 and LCOV >= 2.3-1)
#===============================================================================
GCC_VERSION := $(shell gcc -dumpfullversion -dumpversion)
GCC_OK := $(shell [ "$$(printf '%s\n' '14.2.0' '$(GCC_VERSION)' | sort -V | head -n1)" = '14.2.0' ] && echo 1 || echo 0)

LCOV_VERSION := $(shell lcov --version 2>/dev/null | grep -o '[0-9]\+\.[0-9]\+\(-[0-9]\+\)\?' | head -n1)
LCOV_OK := $(shell [ "$$(printf '%s\n' '2.3-1' '$(LCOV_VERSION)' | sort -V | head -n1)" = '2.3-1' ] && echo 1 || echo 0)

$(info GCC_VERSION is $(GCC_VERSION))
$(info GCC_OK is $(GCC_OK))
$(info LCOV_VERSION is $(LCOV_VERSION))
$(info LCOV_OK is $(LCOV_OK))

#===============================================================================
# Compiler and Flags
#===============================================================================
CC       = gcc
COND_COVERAGE_FLAG := $(if $(and $(findstring 1,$(GCC_OK)),$(findstring 1,$(LCOV_OK))),-fcondition-coverage,)
CFLAGS   = -Wall -Wextra -O0 -g -fprofile-arcs -ftest-coverage $(COND_COVERAGE_FLAG) -DUNIT_TEST
LDFLAGS  = -fprofile-arcs -ftest-coverage $(COND_COVERAGE_FLAG) -lcunit -lssl -lcrypto -lpthread -lncurses -lpanel

#===============================================================================
# Directories
#===============================================================================
SRC_DIR         = ../src
COMMON_INCLUDES = $(SRC_DIR)/common_includes
DASHBOARD_DIR   = $(SRC_DIR)/dashboard
ICLUSTER_DIR    = $(SRC_DIR)/instrument_cluster
BCM_DIR         = $(SRC_DIR)/bcm
POWERTRAIN_DIR  = $(SRC_DIR)/powertrain

TEST_DIR    = .
UNIT_DIR    = $(TEST_DIR)/unit
FEATURE_DIR = $(TEST_DIR)/feature

OBJ_DIR = obj
BIN_DIR = bin

#===============================================================================
# Source Files
#===============================================================================
# 1) Library code, excluding can_socket.c and panels.c so we can link it selectively
REAL_LIB_SOURCES = \
  $(COMMON_INCLUDES)/logging.c \
  $(DASHBOARD_DIR)/dashboard_func.c \
  $(ICLUSTER_DIR)/instrument_cluster_func.c \
  $(BCM_DIR)/bcm_func.c \
  $(POWERTRAIN_DIR)/powertrain_func.c \
  $(POWERTRAIN_DIR)/can_comms.c

# 2) The real can_socket source (compiled when we want real code)
REAL_CAN_SOURCE = \
  $(COMMON_INCLUDES)/can_socket.c

# 3) A mock can_socket for tests that need to stub out can_socket
MOCK_CAN_SOURCE = \
  $(UNIT_DIR)/mock_can_socket.c

# 4) A mock for dashboard functions
MOCK_NCURSES = \
  $(UNIT_DIR)/mock_ncurses.c

# 5) Unit test files (each with its own main)
UNIT_SOURCES = \
  $(UNIT_DIR)/test_logging.c \
  $(UNIT_DIR)/test_dashboard.c \
  $(UNIT_DIR)/test_instrument_cluster.c \
  $(UNIT_DIR)/test_bcm.c \
  $(UNIT_DIR)/test_powertrain.c \
  $(UNIT_DIR)/test_can_socket.c

# 5) Feature tests (if any)
FEATURE_SOURCES = \
  $(FEATURE_DIR)/test_feature_x.c

#===============================================================================
# Object Lists
#===============================================================================
REAL_LIB_OBJECTS  = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(REAL_LIB_SOURCES)))
REAL_CAN          = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(REAL_CAN_SOURCE)))
MOCK_CAN          = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(MOCK_CAN_SOURCE)))
MOCK_UI	          = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(MOCK_NCURSES)))

UNIT_OBJECTS      = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(UNIT_SOURCES)))
FEATURE_OBJECTS   = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(FEATURE_SOURCES)))

#===============================================================================
# Executables (one per test_*.c)
#===============================================================================
# We'll keep them separate, so we can selectively link mock vs real can_socket.

UNIT_TEST_LOGGING       = $(BIN_DIR)/test_logging
UNIT_TEST_DASHBOARD     = $(BIN_DIR)/test_dashboard
UNIT_TEST_INSTRUMENT    = $(BIN_DIR)/test_instrument_cluster
UNIT_TEST_BCM           = $(BIN_DIR)/test_bcm
UNIT_TEST_POWERTRAIN    = $(BIN_DIR)/test_powertrain
UNIT_TEST_CAN_SOCKET    = $(BIN_DIR)/test_can_socket

FEATURE_TEST_X          = $(BIN_DIR)/test_feature_x

# We'll build these for "make all"
UNIT_TESTS = \
  $(UNIT_TEST_LOGGING) \
  $(UNIT_TEST_DASHBOARD) \
  $(UNIT_TEST_INSTRUMENT) \
  $(UNIT_TEST_BCM) \
  $(UNIT_TEST_POWERTRAIN) \
  $(UNIT_TEST_CAN_SOCKET)

FEATURE_TESTS = \
  $(FEATURE_TEST_X)

.PHONY: all
all: $(UNIT_TESTS)
# If you also want to build feature tests by default, you can do:
# all: $(UNIT_TESTS) $(FEATURE_TESTS)

# Ensure directories exist
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

#===============================================================================
# vpath
#===============================================================================
vpath %.c \
  $(COMMON_INCLUDES) \
  $(DASHBOARD_DIR) \
  $(ICLUSTER_DIR) \
  $(BCM_DIR) \
  $(POWERTRAIN_DIR) \
  $(UNIT_DIR) \
  $(FEATURE_DIR)

#===============================================================================
# Compilation Rule
#===============================================================================
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) \
	  -I$(COMMON_INCLUDES) \
	  -I$(DASHBOARD_DIR) \
	  -I$(ICLUSTER_DIR) \
	  -I$(BCM_DIR) \
	  -I$(POWERTRAIN_DIR) \
	  -I$(UNIT_DIR) \
	  -I$(FEATURE_DIR) \
	-c $< -o $@

#===============================================================================
# Linking
#===============================================================================
# test_logging: uses real can_socket
$(UNIT_TEST_LOGGING): $(REAL_LIB_OBJECTS) $(REAL_CAN) $(MOCK_UI) $(OBJ_DIR)/test_logging.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# test_dashboard: uses the mock can_socket and mock_ui
$(UNIT_TEST_DASHBOARD): $(REAL_LIB_OBJECTS) $(MOCK_CAN) $(MOCK_UI) $(OBJ_DIR)/test_dashboard.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# test_instrument_cluster: uses the mock can_socket
$(UNIT_TEST_INSTRUMENT): $(REAL_LIB_OBJECTS) $(MOCK_CAN) $(MOCK_UI) $(OBJ_DIR)/test_instrument_cluster.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# test_bcm: uses the mock can_socket
$(UNIT_TEST_BCM): $(REAL_LIB_OBJECTS) $(MOCK_CAN) $(MOCK_UI) $(OBJ_DIR)/test_bcm.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# test_powertrain: uses the mock can_socket
$(UNIT_TEST_POWERTRAIN): $(REAL_LIB_OBJECTS) $(MOCK_CAN) $(MOCK_UI) $(OBJ_DIR)/test_powertrain.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# test_can_socket: specifically tests real can_socket
$(UNIT_TEST_CAN_SOCKET): $(REAL_LIB_OBJECTS) $(REAL_CAN) $(MOCK_UI) $(OBJ_DIR)/test_can_socket.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# If you have a feature test
# $(FEATURE_TEST_X): $(REAL_LIB_OBJECTS) $(REAL_CAN) $(OBJ_DIR)/test_feature_x.o
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#===============================================================================
# Run Tests
#===============================================================================
.PHONY: test
test: all
	@echo "Running test_logging..."
	@$(UNIT_TEST_LOGGING)
	@echo "Running test_dashboard..."
	@$(UNIT_TEST_DASHBOARD)
	@echo "Running test_instrument_cluster..."
	@$(UNIT_TEST_INSTRUMENT)
	@echo "Running test_can_socket..."
	@$(UNIT_TEST_CAN_SOCKET)
	@echo "Running test_bcm..."
	@$(UNIT_TEST_BCM)
	@echo "Running test_powertrain..."
	@$(UNIT_TEST_POWERTRAIN)
	# If you have feature tests:
	# @echo "Running test_feature_x..."
	# @$(FEATURE_TEST_X)

#===============================================================================
# Coverage
#===============================================================================
.PHONY: coverage
coverage: test
	@echo "Generating coverage data..."
	# Capture coverage with branch info
	lcov --rc branch_coverage=1 --rc mcdc_coverage=1 \
     --capture --directory . \
     --output-file coverage.info \
     --exclude 'tests/*'

	genhtml \
	 --branch-coverage \
	 --mcdc-coverage \
	 coverage.info \
	 --output-directory coverage_report

	@echo "Coverage report -> coverage_report/index.html"

#===============================================================================
# Clean
#===============================================================================
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/* coverage.info coverage_report *.gcno *.gcda
