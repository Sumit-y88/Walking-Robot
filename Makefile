# =============================================================================
# Makefile for Walking Robot with Obstacle Navigation (OpenGL)
# Works on Windows (MinGW), Linux, and macOS
# =============================================================================

CXX      = g++
CXXFLAGS = -O2 -Wall

# Detect Operating System
ifeq ($(OS),Windows_NT)
    # Windows native build using built-in OpenGL & GDI (no external DLLs required)
    CXXFLAGS += -DFREEGLUT_STATIC -Iinclude
    LDFLAGS   = -lopengl32 -lglu32 -lgdi32 -static-libgcc -static-libstdc++
    SOURCES   = main.cpp glut_compat.cpp
    TARGET    = robot.exe
    RM        = del /Q
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        # macOS configuration
        LDFLAGS = -framework OpenGL -framework GLUT
        SOURCES = main.cpp
        TARGET  = robot
        RM      = rm -f
    else
        # Linux / Ubuntu configuration
        LDFLAGS = -lglut -lGL -lGLU
        SOURCES = main.cpp
        TARGET  = robot
        RM      = rm -f
    endif
endif

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
ifeq ($(OS),Windows_NT)
	$(TARGET)
else
	./$(TARGET)
endif

clean:
	$(RM) $(TARGET) *.o
