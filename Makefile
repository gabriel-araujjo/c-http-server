zip:
	@ zip -r simple-web-server .

all:
	@ rm -rf build
	@ mkdir build
	@ gcc -v -Wall -std=c99 src/generic_types.c src/http.c src/db.c src/main.c -o build/simple-web-server

clean:
	@ rm -rf build
	@ mkdir build

db_main: clean
	@ gcc -v -Wall -std=c99 src/generic_types.c src/db.c src/db_main.c -g -o build/main -I /usr/include/postgresql/ -lpq