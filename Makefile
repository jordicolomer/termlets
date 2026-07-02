# Compiler and flags
CC := gcc
#CFLAGS := -Wall -Wextra -Werror -std=c11 -g
CFLAGS := -Wall -Wextra -std=c11 -g -I/opt/homebrew/opt/libvterm/include
LDFLAGS := -L/opt/homebrew/opt/libvterm/lib -lvterm -lpthread

# Project name
TARGET := main

# Source and object files
SRC := main.c logger.c buffer.c ansi_term.c window.c frame.c file_manager.c slider.c taskbar.c terminal.c utils.c vterm_terminal.c ansi256.c tabs.c editor.c menu.c
OBJ := $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET)

# Rebuild everything
rebuild: clean all

# Run the program
run: $(TARGET)
	./$(TARGET)

# Test targets
TEST_SRC := tests/test_lambda.c
TEST_TARGET := tests/test_lambda
TEST_OBJ := lambda.o

$(TEST_TARGET): $(TEST_SRC) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_lambda: $(TEST_TARGET)
	./$(TEST_TARGET)

# Phony targets
.PHONY: all clean rebuild run test_lambda
