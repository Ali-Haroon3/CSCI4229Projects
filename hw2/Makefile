# Makefile for HW2 
# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2 -std=c11

# Target executable
EXE = hw2

# Source files
SRC = hw2.c

# Platform specific libraries
UNAME = $(shell uname)
ifeq "$(UNAME)" "Darwin"
	# macOS - Use frameworks
	LIBS = -framework OpenGL -framework GLUT
	CFLAGS += -Wno-deprecated-declarations
else
	ifeq "$(UNAME)" "CYGWIN"
		# Windows/Cygwin
		LIBS = -lopengl32 -lglu32 -lglut32
	else
		# Linux
		LIBS = -lglut -lGLU -lGL -lm
	endif
endif

# Default target
$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(EXE) $(LIBS)

# Clean target
clean:
	rm -f $(EXE) *.o *~

# Phony targets
.PHONY: clean
