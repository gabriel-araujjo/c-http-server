#include <stdio.h>
#include <stdlib.h>
#include "http.h"
#include "db.h"
#include "api/repository.h"
#include "api/api.h"

int main(int argc, char** argv) {
    int max_connections = 2000;
    int thread_pool_size = 400;
    int db_conn_pool_size = 50;

    if (argc == 4) {
        max_connections = atoi(argv[1]);
        thread_pool_size = atoi(argv[2]);
        db_conn_pool_size = atoi(argv[3]);
    }

    printf("Using args - max_connections=%d, http_thread_pool_size=%d, db_conn_pool_size=%d\n", max_connections, thread_pool_size, db_conn_pool_size);

    db_conn_pool* p = db_conn_pool_create(50, "dbname=db user=admin password=admin host=db port=5432");
    printf("created conn pool: %p\n", (void*)p);

    client_repository_t* r = client_repository_create(p);
    printf("created repository: %p\n", (void*)r);

    client_service_t* c = client_service_create(r);
    printf("created client service: %p\n", (void*)c);

    extract_service_t* e = extract_service_create(r);
    printf("created extract service: %p\n", (void*)c);

    FILE* f = fopen("./logs.txt", "a+");
    if (!f) {
        printf("Error opening log file\n");
        return 0;
    }

    http_server* s = http_create_server(f, 8090, max_connections, thread_pool_size);
    http_add_route(s, "POST /clientes/([0-9]{1,})/transacoes", c, c->handler);
    http_add_route(s, "GET /clientes/([0-9]{1,})/extrato", c, e->handler);
    http_serve(f, s);
    free(s);
    fclose(f);

    // TODO: I could manually listen for a SIGTERM signal and free allocated 
    // resources here for graceful finishing, but the OS will do that after process
    // termination. The only scenario that makes sense is using SIGTERM to finish 
    // database connections gracefully
    return 0;
}
