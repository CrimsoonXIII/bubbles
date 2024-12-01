all: myGLutil/myGLutil.o
	g++ -o main main.cpp myGLutil/myGLutil.o -I myGLutil -lGL -lglfw -lGLEW -std=c++20
myGLutil/myGLutil.o:
	cd myGLutil && make