CC = gcc
CFLAGS = -Wall -Wextra -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
LDLFLAGS = -lssl -lcrypto

# Directories
SRC_DIR = .
COMMON_DIR = $(SRC_DIR)/common_includes
BIN_DIR = ../bin

# Ensure bin directory exists
$(shell mkdir -p $(BIN_DIR))

# Common object files (from common_includes)
COMMON_OBJ = $(BIN_DIR)/can_socket.o $(BIN_DIR)/logging.o

TARGETS = \
  $(BIN_DIR)/sender \
  $(BIN_DIR)/receiver \
  $(BIN_DIR)/instrument_cluster \
  $(BIN_DIR)/dashboard

all: $(TARGETS)

######################################################
# Common_includes object compilation
######################################################
$(BIN_DIR)/can_socket.o: $(COMMON_DIR)/can_socket.c $(COMMON_DIR)/can_socket.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $(COMMON_DIR)/can_socket.c -o $@

$(BIN_DIR)/logging.o: $(COMMON_DIR)/logging.c $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $(COMMON_DIR)/logging.c -o $@

######################################################
# Sender
######################################################
$(BIN_DIR)/sender.o: sender.c $(COMMON_DIR)/can_socket.h $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c sender.c -o $@

$(BIN_DIR)/sender: $(BIN_DIR)/sender.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

######################################################
# Receiver
######################################################
$(BIN_DIR)/receiver.o: receiver.c $(COMMON_DIR)/can_socket.h $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c receiver.c -o $@

$(BIN_DIR)/receiver: $(BIN_DIR)/receiver.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

######################################################
# Instrument Cluster
######################################################
# If instrument_cluster.c is inside src/instrument_cluster folder:
INST_CLUST_DIR = $(SRC_DIR)/instrument_cluster

$(BIN_DIR)/instrument_cluster.o: $(INST_CLUST_DIR)/instrument_cluster.c \
                                 $(COMMON_DIR)/can_socket.h \
                                 $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $(INST_CLUST_DIR)/instrument_cluster.c -o $@

$(BIN_DIR)/instrument_cluster: $(BIN_DIR)/instrument_cluster.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

######################################################
# Dashboard
######################################################
# If dashboard.c is inside src/dashboard folder:
DASH_DIR = $(SRC_DIR)/dashboard

$(BIN_DIR)/dashboard.o: $(DASH_DIR)/dashboard.c $(COMMON_DIR)/can_socket.h $(COMMON_DIR)/logging.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $(DASH_DIR)/dashboard.c -o $@

$(BIN_DIR)/dashboard: $(BIN_DIR)/dashboard.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLFLAGS)

######################################################
# Clean and Run
######################################################
clean:
	rm -rf $(BIN_DIR)/*

run: all
	$(BIN_DIR)/sender & $(BIN_DIR)/receiver
