#include <stdio.h>
#include <stdlib.h>
#include "http.h"
#include "api.h"

int main(int argc, char** argv) {
    client_service* c = client_service_create();

    FILE* f = fopen("./logs.txt", "a+");
    if (!f) {
        printf("Error opening log file\n");
        return 0;
    }
    fprintf(f, "Creating server\n");
    http_server* s = http_create_server(f, 8090, 2);
    fprintf(f, "Created server\n");
    http_add_route(s, "POST /clientes/([0-9]{1,})/transacoes", c->handler);
    http_serve(f, s);
    free(s);
    fclose(f);
    return 0;
}