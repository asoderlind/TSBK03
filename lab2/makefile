# set this variable to the director in which you saved the common files
COMMONDIR = ../common/
LIBDIR = lib/


CC = gcc
CXX = g++
CFLAGS = -Wall -DGL_GLEXT_PROTOTYPES -Wno-deprecated-declarations
CXXFLAGS = $(CFLAGS) -std=c++17
LDFLAGS = -lm -framework OpenGL -framework Cocoa -framework CoreGraphics -Wno-deprecated-declarations

C_SOURCES = \
$(COMMONDIR)GL_utilities.c \
$(COMMONDIR)LoadTGA.c \
$(COMMONDIR)/MicroGlut.m \

CPP_SOURCES = $(shell find $(LIBDIR) -type f -name '*.cpp')

# Object files
C_OBJECTS := $(C_SOURCES:.c=.o)
CPP_OBJECTS := $(CPP_SOURCES:.cpp=.o)

lab2-A1: skinning.o $(C_OBJECTS) $(CPP_OBJECTS)
	$(CXX) -o lab2-A1 skinning.o $(C_OBJECTS) $(CPP_OBJECTS) $(LDFLAGS)

lab2-A1.o: lab2-A1.cpp
	$(CXX) $(CXXFLAGS) -I $(COMMONDIR) -I $(LIBDIR) -c lab2-A1.cpp -o skinning.o

lab2-A2: skinning.o $(C_OBJECTS) $(CPP_OBJECTS)
	$(CXX) -o lab2-A2 skinning.o $(C_OBJECTS) $(CPP_OBJECTS) $(LDFLAGS)

lab2-A2.o: lab2-A2.cpp
	$(CXX) $(CXXFLAGS) -I $(COMMONDIR) -I $(LIBDIR) -c lab2-A2.cpp -o skinning.o

lab2-2: skinning2.o $(C_OBJECTS) $(CPP_OBJECTS)
	$(CXX) -o lab2-2 skinning2.o $(C_OBJECTS) $(CPP_OBJECTS) $(LDFLAGS)

lab2-2.o: skinning2.cpp
	$(CXX) $(CXXFLAGS) -I $(COMMONDIR) -I $(LIBDIR) -c skinning2.cpp -o skinning2.o

# Pattern rule for C source files
%.o: %.c
	$(CC) $(CFLAGS) -I $(COMMONDIR) -I $(LIBDIR) -c $< -o $@

# Pattern rule for C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I $(COMMONDIR) -I $(LIBDIR) -c $< -o $@

clean :
	rm lab2-1 skinning.o lab2-2 skinning2.o $(CPP_OBJECTS)
