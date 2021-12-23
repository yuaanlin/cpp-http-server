run:
	mkdir -p build
	clang++ -std=c++20 server.cpp -o build/server
	./build/server
	
clean:
	rm -rf build