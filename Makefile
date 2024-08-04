# Platform detection
#CC = gcc
#CFLAGS = -Wall -Iinclude
#LDFLAGS = -lglfw -lGL -lm
#MKDIR = mkdir -p
#RM = rm -f
#EXE_EXT =

# Windows-specific settings (using MSYS2 or MinGW)
CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -Iinclude -Iglfw/include
LDFLAGS = -Lglfw\lib-mingw-w64 -lglfw3 -lgdi32 -lopengl32 -lgdiplus
MKDIR = mkdir -p
RM = del /Q

# Directories
SRC_DIR = src
HEADER_DIR = header
OBJ_DIR = obj

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Target executable
TARGET = program$(EXE_EXT)

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
	$(MKDIR) $(OBJ_DIR)

# Clean target to remove generated files
clean:
	$(RM) $(OBJ_DIR)/*.o $(TARGET)
	rmdir $(OBJ_DIR)

# Phony targets
.PHONY: all clean

# Include the dependencies
-include $(OBJS:.o=.d)

# Rule to generate the dependency files
#$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c | $(OBJ_DIR)
#	@set -e; rm -f $@; \
#	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
#	sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
#	rm -f $@.$$$$