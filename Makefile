zip:
	@ zip -r simple-web-server .

all:
	@ rm -rf build
	@ mkdir build
	@ gcc -v -Wall -std=c99 src/generic_types.c src/http.c src/main.c -o build/simple-web-server