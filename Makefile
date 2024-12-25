CC = gcc
CFLAGS = -Wall -Wextra
INCLUDES = -I./lib

SRC_DIR = src
MODULES_DIR = modules
BUILD_DIR = build
LIB_DIR = lib

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(MODULES_DIR)/*.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
EXECUTABLE = database

.PHONY: all clean directories

all: directories $(EXECUTABLE)

directories:
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	@if not exist "$(BUILD_DIR)\$(SRC_DIR)" mkdir "$(BUILD_DIR)\$(SRC_DIR)"
	@if not exist "$(BUILD_DIR)\$(MODULES_DIR)" mkdir "$(BUILD_DIR)\$(MODULES_DIR)"
	@if not exist "data" mkdir "data"

$(EXECUTABLE): $(OBJS)
	$(CC) $(OBJS) -o $@

$(BUILD_DIR)/%.o: %.c
	@if not exist "$(@D)" mkdir "$(@D)"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	del /Q *.o *.exe 2>NUL
	del /Q $(EXECUTABLE).exe 2>NUL
	del /Q $(EXECUTABLE) 2>NUL
	if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
	if exist "data" rmdir /S /Q "data"