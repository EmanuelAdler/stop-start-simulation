#===============================================================================
# Compiler and Flags
#===============================================================================
CC       = gcc
CFLAGS   = -Wall -Wextra -g --coverage
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
# (1) Library source files (no main)
LIB_SOURCES = \
  $(COMMON_INCLUDES)/can_socket.c \
  $(COMMON_INCLUDES)/logging.c \
  $(DASHBOARD_DIR)/dashboard_func.c \
  $(ICLUSTER_DIR)/instrument_cluster_func.c

# (2) Unit test source files (each with its own main)
UNIT_SOURCES = \
  $(UNIT_DIR)/test_logging.c \
  $(UNIT_DIR)/test_dashboard.c \
  $(UNIT_DIR)/test_instrument_cluster.c

# (3) Feature test source files (example)
FEATURE_SOURCES = \
  $(FEATURE_DIR)/test_feature_x.c

#===============================================================================
# Object Files
#===============================================================================
LIB_OBJECTS      = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(LIB_SOURCES)))
UNIT_OBJECTS     = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(UNIT_SOURCES)))
FEATURE_OBJECTS  = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(FEATURE_SOURCES)))

#===============================================================================
# Multiple Test Executables
#===============================================================================
# For each test_X.c file, we'll produce a separate bin/test_X

# 1) Unit test executables
UNIT_TESTS = \
  $(BIN_DIR)/test_logging \
  $(BIN_DIR)/test_dashboard \
  $(BIN_DIR)/test_instrument_cluster

# 2) Feature test executables (if desired)
FEATURE_TESTS = \
  $(BIN_DIR)/test_feature_x

# We'll build all of these when we run "make all"
.PHONY: all
all: $(UNIT_TESTS) # $(FEATURE_TESTS)

# Ensure directories exist
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

#===============================================================================
# vpath for Source Files
#===============================================================================
vpath %.c $(COMMON_INCLUDES) $(DASHBOARD_DIR) $(ICLUSTER_DIR)
vpath %.c $(UNIT_DIR) $(FEATURE_DIR)

#===============================================================================
# Compilation Rule (object files)
#===============================================================================
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) \
	  -I$(COMMON_INCLUDES) -I$(DASHBOARD_DIR) -I$(ICLUSTER_DIR) \
	  -I$(UNIT_DIR) -I$(FEATURE_DIR) \
	-c $< -o $@

#===============================================================================
# Linking Rules (one executable per test_X.c)
#===============================================================================
$(BIN_DIR)/test_logging: $(LIB_OBJECTS) $(OBJ_DIR)/test_logging.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/test_dashboard: $(LIB_OBJECTS) $(OBJ_DIR)/test_dashboard.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/test_instrument_cluster: $(LIB_OBJECTS) $(OBJ_DIR)/test_instrument_cluster.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# $(BIN_DIR)/test_feature_x: $(LIB_OBJECTS) $(OBJ_DIR)/test_feature_x.o
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#===============================================================================
# Test Target
#===============================================================================
.PHONY: test
test: all
	@echo "Running test_logging..."
	@$(BIN_DIR)/test_logging
	@echo "Running test_dashboard..."
	@$(BIN_DIR)/test_dashboard
	@echo "Running test_instrument_cluster..."
	@$(BIN_DIR)/test_instrument_cluster

#===============================================================================
# Coverage
#===============================================================================
.PHONY: coverage
coverage: test
	@echo "Generating coverage data..."
	lcov --rc branch_coverage=1 --capture --directory . --output-file coverage.info
	genhtml --branch-coverage coverage.info --output-directory coverage_report
	@echo "Coverage report -> coverage_report/index.html"

#===============================================================================
# Clean
#===============================================================================
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/* coverage.info coverage_report *.gcno *.gcda
