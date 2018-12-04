CC = g++

# Linux (default)
TARGET = islandDefender3d
CFLAGS = -Wall -I.
LDFLAGS = -lGL -lGLU -lglut -lm ./libSOIL.a

# Windows (cygwin)
ifeq "$(OS)" "Windows_NT"
	TARGET = islandDefender3d.exe
	CFLAGS += -D_WIN32
endif

# OS X
ifeq "$(OSTYPE)" "darwin"
	LDFLAGS = -framework Carbon -framework QuickTime -framework OpenGL -framework GLUT
	CFLAGS = -D__APPLE__ -Wall
endif


SRC = islandDefender3d.cpp
OBJ = islandDefender3d.o


all: $(TARGET)

debug: CFLAGS += -g
debug: all

$(TARGET): $(OBJ)
	@echo linking $(TARGET)
	@$(CC) -o $(TARGET) $(OBJ) $(CFLAGS) $(LDFLAGS)

%.o: %.cpp
	@echo compiling $@ $(CFLAGS)
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY:
clean:
	@echo cleaning $(OBJ)
	@rm $(OBJ)
