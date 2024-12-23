CC = gcc
CFLAGS = -Wall -Wextra
INCLUDES = -I./bpt -I.

SRC_DIR = src
BPT_DIR = bpt
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(BPT_DIR)/*.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
EXECUTABLE = database

.PHONY: all clean directories

all: directories $(EXECUTABLE)

directories:
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	@if not exist "$(BUILD_DIR)\$(SRC_DIR)" mkdir "$(BUILD_DIR)\$(SRC_DIR)"
	@if not exist "$(BUILD_DIR)\$(BPT_DIR)" mkdir "$(BUILD_DIR)\$(BPT_DIR)"

$(EXECUTABLE): $(OBJS)
	$(CC) $(OBJS) -o $@

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	del /Q *.o *.exe 2>NUL
	del /Q $(EXECUTABLE).exe 2>NUL
	del /Q $(EXECUTABLE) 2>NUL
	if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"