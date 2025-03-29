#===============================================================================
# Compiler/Flags
#===============================================================================
CC       = gcc
CFLAGS   = -Wall -Wextra -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
LDLFLAGS = -lssl -lcrypto -lpthread  # libraries used during linking

#===============================================================================
# Directories
#===============================================================================
SRC_DIR        = .
BIN_DIR        = ../bin

COMMON_DIR     = $(SRC_DIR)/common_includes
DASH_DIR       = $(SRC_DIR)/dashboard
INSTR_CLUST_DIR = $(SRC_DIR)/instrument_cluster
BCM_DIR = $(SRC_DIR)/bcm
POWERTRAIN_DIR = $(SRC_DIR)/powertrain

# Ensure the bin/ directory exists
$(shell mkdir -p $(BIN_DIR))

#===============================================================================
# Final Executables
#===============================================================================
TARGETS = \
  $(BIN_DIR)/sender \
  $(BIN_DIR)/receiver \
  $(BIN_DIR)/instrument_cluster \
  $(BIN_DIR)/dashboard \
  $(BIN_DIR)/bcm \
  $(BIN_DIR)/powertrain

all: $(TARGETS)

#===============================================================================
# Common Objects (from common_includes)
#===============================================================================
COMMON_OBJ = \
  $(BIN_DIR)/can_socket.o \
  $(BIN_DIR)/logging.o

# 1) can_socket.o
$(BIN_DIR)/can_socket.o: $(COMMON_DIR)/can_socket.c $(COMMON_DIR)/can_socket.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $< -o $@

# 2) logging.o
$(BIN_DIR)/logging.o: $(COMMON_DIR)/logging.c $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $< -o $@

#===============================================================================
# Sender
#===============================================================================
$(BIN_DIR)/sender.o: $(SRC_DIR)/sender.c \
                     $(COMMON_DIR)/can_socket.h \
                     $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $< -o $@

$(BIN_DIR)/sender: $(BIN_DIR)/sender.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

#===============================================================================
# Receiver
#===============================================================================
$(BIN_DIR)/receiver.o: $(SRC_DIR)/receiver.c \
                       $(COMMON_DIR)/can_socket.h \
                       $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $< -o $@

$(BIN_DIR)/receiver: $(BIN_DIR)/receiver.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

#===============================================================================
# Instrument Cluster
#  - Needs to compile instrument_cluster.c (which contains main())
#  - Also compiles instrument_cluster_func.c
#===============================================================================
INSTR_CLUST_OBJS = \
  $(BIN_DIR)/instrument_cluster.o \
  $(BIN_DIR)/instrument_cluster_func.o

# (a) instrument_cluster.o (has main)
$(BIN_DIR)/instrument_cluster.o: $(INSTR_CLUST_DIR)/instrument_cluster.c \
                                 $(INSTR_CLUST_DIR)/instrument_cluster_func.h \
                                 $(COMMON_DIR)/can_socket.h \
                                 $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(INSTR_CLUST_DIR) -c $< -o $@

# (b) instrument_cluster_func.o (auxiliary logic)
$(BIN_DIR)/instrument_cluster_func.o: $(INSTR_CLUST_DIR)/instrument_cluster_func.c \
                                      $(INSTR_CLUST_DIR)/instrument_cluster_func.h \
                                      $(COMMON_DIR)/can_socket.h \
                                      $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(INSTR_CLUST_DIR) -c $< -o $@

# (c) link final instrument_cluster
$(BIN_DIR)/instrument_cluster: $(INSTR_CLUST_OBJS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

#===============================================================================
# Dashboard
#  - Needs to compile dashboard.c (which contains main())
#  - Also compiles dashboard_func.c
#===============================================================================
DASH_OBJS = \
  $(BIN_DIR)/dashboard.o \
  $(BIN_DIR)/dashboard_func.o

# (a) dashboard.o (has main)
$(BIN_DIR)/dashboard.o: $(DASH_DIR)/dashboard.c \
                        $(DASH_DIR)/dashboard_func.h \
                        $(COMMON_DIR)/can_socket.h \
                        $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(DASH_DIR) -c $< -o $@

# (b) dashboard_func.o (auxiliary logic)
$(BIN_DIR)/dashboard_func.o: $(DASH_DIR)/dashboard_func.c \
                             $(DASH_DIR)/dashboard_func.h \
                             $(COMMON_DIR)/can_socket.h \
                             $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(DASH_DIR) -c $< -o $@

# (c) link final dashboard
$(BIN_DIR)/dashboard: $(DASH_OBJS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

#===============================================================================
# BCM (Body Control Module)
#  - Needs to compile bcm.c (which contains main())
#  - Also compiles bcm_func.c
#===============================================================================
BCM_OBJS = \
  $(BIN_DIR)/bcm.o \
  $(BIN_DIR)/bcm_func.o

# (a) bcm.o (has main)
$(BIN_DIR)/bcm.o: $(BCM_DIR)/bcm.c \
                        $(BCM_DIR)/bcm_func.h \
                        $(COMMON_DIR)/can_socket.h \
                        $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(BCM_DIR) -c $< -o $@

# (b) bcm_func.o (auxiliary logic)
$(BIN_DIR)/bcm_func.o: $(BCM_DIR)/bcm_func.c \
                             $(BCM_DIR)/bcm_func.h \
                             $(COMMON_DIR)/can_socket.h \
                             $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(BCM_DIR) -c $< -o $@

# (c) link final bcm
$(BIN_DIR)/bcm: $(BCM_OBJS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

#===============================================================================
# Powertrain
#  - Needs to compile powertrain.c (which contains main())
#  - Also compiles powertrain_func.c
#===============================================================================
POWERTRAIN_OBJS = \
  $(BIN_DIR)/powertrain.o \
  $(BIN_DIR)/can_comms.o \
  $(BIN_DIR)/battery.o

# (a) powertrain.o (has main)
$(BIN_DIR)/powertrain.o: $(POWERTRAIN_DIR)/powertrain.c \
                        $(POWERTRAIN_DIR)/powertrain.h \
                        $(POWERTRAIN_DIR)/battery.h \
                        $(POWERTRAIN_DIR)/can_comms.h \
                        $(COMMON_DIR)/can_socket.h \
                        $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(POWERTRAIN_DIR) -c $< -o $@

# (b) can_comms.o
$(BIN_DIR)/can_comms.o: $(POWERTRAIN_DIR)/can_comms.c \
                        $(POWERTRAIN_DIR)/powertrain.h \
                        $(POWERTRAIN_DIR)/can_comms.h \
                        $(COMMON_DIR)/can_socket.h \
                        $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(POWERTRAIN_DIR) -c $< -o $@

# (b) battery.o
$(BIN_DIR)/battery.o: $(POWERTRAIN_DIR)/battery.c \
                        $(POWERTRAIN_DIR)/powertrain.h \
                        $(POWERTRAIN_DIR)/battery.h \
                        $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(POWERTRAIN_DIR) -c $< -o $@

# (c) link final powertrain
$(BIN_DIR)/powertrain: $(POWERTRAIN_OBJS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

#===============================================================================
# Clean and Run
#===============================================================================
clean:
	rm -rf $(BIN_DIR)/*

run: all
	$(BIN_DIR)/sender & \
	$(BIN_DIR)/receiver
