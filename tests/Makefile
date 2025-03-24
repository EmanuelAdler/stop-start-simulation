#===============================================================================
# Compiler and Flags
#===============================================================================
CC       = gcc
CFLAGS   = -Wall -Wextra -g --coverage -DUNIT_TEST
LDFLAGS  = -lcunit -lssl -lcrypto -lpthread --coverage

#===============================================================================
# Directories
#===============================================================================
SRC_DIR         = ../src
COMMON_INCLUDES = $(SRC_DIR)/common_includes
DASHBOARD_DIR   = $(SRC_DIR)/dashboard
ICLUSTER_DIR    = $(SRC_DIR)/instrument_cluster

TEST_DIR    = .
UNIT_DIR    = $(TEST_DIR)/unit
FEATURE_DIR = $(TEST_DIR)/feature

OBJ_DIR = obj
BIN_DIR = bin

#===============================================================================
# Source Files
#===============================================================================
# (1) Library source files, excluding can_socket.c so we can handle it separately
REAL_LIB_SOURCES = \
  $(COMMON_INCLUDES)/logging.c \
  $(DASHBOARD_DIR)/dashboard_func.c \
  $(ICLUSTER_DIR)/instrument_cluster_func.c

# The real can_socket
REAL_CAN_SOURCE = \
  $(COMMON_INCLUDES)/can_socket.c

# The mock can_socket for stubbing
MOCK_CAN_SOURCE = \
  $(UNIT_DIR)/mock_can_socket.c

# (2) Unit test source files (each with its own main)
#     Note that we do NOT put mock_can_socket.c here anymore;
#     we separate it above so we can link it selectively.
UNIT_SOURCES = \
  $(UNIT_DIR)/test_logging.c \
  $(UNIT_DIR)/test_dashboard.c \
  $(UNIT_DIR)/test_instrument_cluster.c

# (3) Feature test sources (if any)
FEATURE_SOURCES = \
  $(FEATURE_DIR)/test_feature_x.c

#===============================================================================
# Object Files
#===============================================================================
REAL_LIB_OBJECTS  = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(REAL_LIB_SOURCES)))
REAL_CAN          = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(REAL_CAN_SOURCE)))
MOCK_CAN          = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(MOCK_CAN_SOURCE)))

UNIT_OBJECTS      = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(UNIT_SOURCES)))
FEATURE_OBJECTS   = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(FEATURE_SOURCES)))

#===============================================================================
# Test executables
#===============================================================================
# We'll produce one test binary per test_*.c

UNIT_TEST_LOGGING          = $(BIN_DIR)/test_logging
UNIT_TEST_DASHBOARD        = $(BIN_DIR)/test_dashboard
UNIT_TEST_INSTRUMENT       = $(BIN_DIR)/test_instrument_cluster

FEATURE_TEST_X             = $(BIN_DIR)/test_feature_x

# Let's group them for a "make all"
UNIT_TESTS = $(UNIT_TEST_LOGGING) $(UNIT_TEST_DASHBOARD) $(UNIT_TEST_INSTRUMENT)
FEATURE_TESTS = $(FEATURE_TEST_X)

.PHONY: all
all: $(UNIT_TESTS) 
# If you want to build feature tests too, add them:
# all: $(UNIT_TESTS) $(FEATURE_TESTS)

# Ensure directories exist
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

#===============================================================================
# vpath for Source Files
#===============================================================================
vpath %.c \
  $(COMMON_INCLUDES) \
  $(DASHBOARD_DIR) \
  $(ICLUSTER_DIR) \
  $(UNIT_DIR) \
  $(FEATURE_DIR)

#===============================================================================
# Compilation Rule (object files)
#===============================================================================
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) \
	  -I$(COMMON_INCLUDES) -I$(DASHBOARD_DIR) -I$(ICLUSTER_DIR) \
	  -I$(UNIT_DIR) -I$(FEATURE_DIR) \
	-c $< -o $@

#===============================================================================
# Linking
#===============================================================================
# We link the real can_socket for test_logging and test_dashboard:
$(UNIT_TEST_LOGGING): $(REAL_LIB_OBJECTS) $(REAL_CAN) $(OBJ_DIR)/test_logging.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(UNIT_TEST_DASHBOARD): $(REAL_LIB_OBJECTS) $(MOCK_CAN) $(OBJ_DIR)/test_dashboard.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# But for test_instrument_cluster, we *stub* out can_socket:
$(UNIT_TEST_INSTRUMENT): $(REAL_LIB_OBJECTS) $(MOCK_CAN) $(OBJ_DIR)/test_instrument_cluster.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# If you want feature tests similarly:
# $(FEATURE_TEST_X): $(REAL_LIB_OBJECTS) $(REAL_CAN) $(OBJ_DIR)/test_feature_x.o
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#===============================================================================
# Test Target
#===============================================================================
.PHONY: test
test: all
	@echo "Running test_logging..."
	@$(UNIT_TEST_LOGGING)
	@echo "Running test_dashboard..."
	@$(UNIT_TEST_DASHBOARD)
	@echo "Running test_instrument_cluster..."
	@$(UNIT_TEST_INSTRUMENT)
	# If you want feature tests:
	# @echo "Running test_feature_x..."
	# @$(FEATURE_TEST_X)

#===============================================================================
# Coverage
#===============================================================================
.PHONY: coverage
coverage: test
	@echo "Generating coverage data..."
	lcov --rc branch_coverage=1 --capture --directory . --output-file coverage.info
	lcov --remove coverage.info "**/tests/*" --output-file coverage.info
	genhtml --branch-coverage coverage.info --output-directory coverage_report
	@echo "Coverage report -> coverage_report/index.html"

#===============================================================================
# Clean
#===============================================================================
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/* coverage.info coverage_report *.gcno *.gcda
