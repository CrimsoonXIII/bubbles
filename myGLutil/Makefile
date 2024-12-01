myGLutil.o: window.o drawable.o shader.o camera.o mesh.o vao.o
	ld -r -o myGLutil.o obj/window.o obj/drawable.o obj/shader.o obj/camera.o obj/mesh.o obj/vao.o
test: myGLutil.o
	g++ test.cpp myGLutil.o -o test -lGL -lglfw -lGLEW -std=c++20
window.o: src/window.cpp include/window.hpp
	mkdir -p obj && g++ -c src/window.cpp -o obj/window.o -I include -std=c++20
drawable.o: src/drawable.cpp include/drawable.hpp
	mkdir -p obj && g++ -c src/drawable.cpp -o obj/drawable.o -I include -O3 -std=c++20
vao.o: src/vao.cpp include/vao.hpp
	mkdir -p obj && g++ -c src/vao.cpp -o obj/vao.o -I include -O3 -std=c++20
shader.o: src/shader.cpp include/shader.hpp
	mkdir -p obj && g++ -c src/shader.cpp -o obj/shader.o -I include -O3 -std=c++20
camera.o: src/camera.cpp include/camera.hpp
	mkdir -p obj && g++ -c src/camera.cpp -o obj/camera.o -I include -O3 -std=c++20
mesh.o: src/mesh.cpp include/mesh.hpp
	mkdir -p obj && g++ -c src/mesh.cpp -o obj/mesh.o -I include -O3 -std=c++20