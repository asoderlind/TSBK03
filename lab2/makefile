# set this variable to the director in which you saved the common files
commondir = ../common/

all : lab2-1 lab2-2

lab2-1 : skinning.cpp $(commondir)GL_utilities.c $(commondir)Linux/MicroGlut.c
	g++ -Wall -o skinning -I$(commondir) -I$(commondir)/Linux -DGL_GLEXT_PROTOTYPES skinning.cpp $(commondir)GL_utilities.c $(commondir)Linux/MicroGlut.c -lXt -lX11 -lGL -lm

lab2-2 : skinning2.cpp $(commondir)GL_utilities.c $(commondir)Linux/MicroGlut.c
	g++ -Wall -o skinning2 -I$(commondir) -I$(commondir)/Linux -DGL_GLEXT_PROTOTYPES skinning2.cpp $(commondir)GL_utilities.c  $(commondir)Linux/MicroGlut.c -lXt -lX11 -lGL -lm

clean :
	rm skinning skinning2
