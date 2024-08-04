# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lglfw -lGL -lm

# Directories
SRC_DIR = src
HEADER_DIR = header
OBJ_DIR = obj

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Target executable
TARGET = program

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean target to remove generated files
clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)
	rmdir $(OBJ_DIR)

# Phony targets
.PHONY: all clean

# Include the dependencies
-include $(OBJS:.o=.d)

# Rule to generate the dependency files
$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$