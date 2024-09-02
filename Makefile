zip:
	@ rm simple-web-server.zip
	@ zip -r simple-web-server .

build: clean
	@ gcc -Wall -Wpedantic -Wextra -std=c99 src/generic_types.c src/queue.c src/http.c src/api/api.c src/api/api_types.c src/api/repository.c src/api/utils.c src/db.c src/main.c -o build/main -I /usr/include/postgresql/ -lpq -lpthread -lm

docker_build: clean build
	@ docker login docker.io/lfmtsml
	@ docker build --platform linux/amd64 -t c-simple-web-server .
	@ docker tag c-simple-web-server lfmtsml/c-simple-web-server:latest
	@ docker push lfmtsml/c-simple-web-server:latest

build_safe: clean
	@ gcc -v -Wall -std=c99 -fsanitize=address -static-libasan -g src/generic_types.c src/queue.c src/http.c src/api/api.c src/api/api_types.c src/api/repository.c src/api/utils.c src/db.c src/main.c -o build/main -I /usr/include/postgresql/ -lpq -lpthread -lm

run_safe: build_safe
	@ ./build/main

clean:
	@ rm -rf build
	@ mkdir build

run: clean build
	@ ./build/main

debug: clean build
	@ gdb ./build/main

request_not_found:
	@ curl -v --location --request POST 'http://localhost:8090/clientes/123/transacoes' --header 'Content-Type: application/json' --data-raw '{"valor":1000000,"tipo":"c","descricao":"teste test"}'

request_ok_credit:
	@ curl -v --location --request POST 'http://localhost:8090/clientes/1/transacoes' --header 'Content-Type: application/json' --data-raw '{"valor":1500,"tipo":"c","descricao":"teste test"}'

request_ok_debit:
	@ curl -v --location --request POST 'http://localhost:8090/clientes/1/transacoes' --header 'Content-Type: application/json' --data-raw '{"valor":1500,"tipo":"d","descricao":"teste test"}'

request_ok_list:
	@ curl -v --location --request GET 'http://localhost:8090/clientes/1/extrato' --header 'Content-Type: application/json'

request_not_found_list:
	@ curl -v --location --request GET 'http://localhost:8090/clientes/123/extrato' --header 'Content-Type: application/json'

request_simple:
	@ curl -v --location --request POST 'http://localhost:8090/clientes/123/transacoes' --header 'Content-Type: application/json' --data-raw '{}'


sanity: request request request

loadtest:
	@ docker run --rm -i grafana/k6 run - <load/api-basic-test.js

db_main: clean
	@ gcc -v -Wall -std=c99 src/generic_types.c src/db.c src/db_main.c -g -o build/main -I /usr/include/postgresql/ -lpq

date_main: clean
	@ gcc -v -Wall -std=c99 src/date_main.c -g -o build/main -lm

regex_main: clean
	@ gcc -v -Wall -std=c99 src/regex_main.c -g -o build/main
