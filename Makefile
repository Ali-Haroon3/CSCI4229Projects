# Makefile for Cave Dweller with Tessellation
# Supports Linux, macOS, and Windows (MinGW)

# Compiler
CC = gcc

# Base flags
CFLAGS = -Wall -O3 -std=c11
LDFLAGS = -lm

# Platform detection
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    # Linux
    CFLAGS += -D_GNU_SOURCE
    LDFLAGS += -lGL -lGLEW -lglut
    TARGET = cave_dweller
endif

ifeq ($(UNAME_S),Darwin)
    # macOS
    CFLAGS += -Wno-deprecated-declarations -DGL_SILENCE_DEPRECATION
    LDFLAGS += -framework OpenGL -framework GLUT -framework CoreFoundation
    TARGET = cave_dweller
endif

ifeq ($(OS),Windows_NT)
    # Windows
    CFLAGS += -D_WIN32
    LDFLAGS += -lopengl32 -lglew32 -lfreeglut -lglu32
    TARGET = cave_dweller.exe
endif

# Source files
SOURCES = final.c shaders.c cave.c lighting.c ui.c
HEADERS = shaders.h cave.h lighting.h ui.h
OBJECTS = $(SOURCES:.c=.o)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Clean
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install (Linux only)
install: $(TARGET)
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		sudo cp $(TARGET) /usr/local/bin/; \
		echo "Installed to /usr/local/bin/$(TARGET)"; \
	else \
		echo "Install target only supported on Linux"; \
	fi

# Run
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean debug install run
