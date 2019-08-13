CXX = cl
CXXFLAGS = /EHsc /c /MD
INCLUDES = \
	/I C:\Users\yipin\Documents\CPP_Libraries\glut\include \
	/I C:\Users\yipin\Documents\CPP_Libraries\glew\include \
	/I "C:\Users\yipin\Desktop\soil\src"
	
	
LIBS = \
	/LIBPATH:C:\Users\yipin\Documents\CPP_Libraries\glut\lib \
	/LIBPATH:C:\Users\yipin\Documents\CPP_Libraries\glew\lib\Release\Win32 \
	/LIBPATH:"C:\Users\yipin\Desktop\soil\projects\VC9\Debug" \
	freeglut.lib glew32.lib SOIL.lib
	
	
all: project

project: islandDefender3d.obj
	$(CXX) /EHsc /Fetest.exe $** /link $(LIBS)
	
islandDefender3d.obj: islandDefender3d.cpp
	$(CXX) $(CXXFLAGS) islandDefender3d.cpp $(INCLUDES)
	
clean:
	del *.exe *.obj
	

# $** is equivalent to $^
