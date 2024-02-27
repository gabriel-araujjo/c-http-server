#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <pthread.h>

#include "generic_types.h"
#include "db.h"

db_conn_pool* p = NULL;

void* do_operation(void* args);

void* do_operation(void* args) {
    long int* tid = (long int*)args;
    PGconn* conn = db_conn_pool_pop(p);
    
    char* query = "select * from clientes";

    PGresult* res = PQexec(conn, query);
    ExecStatusType resStatus = PQresultStatus(res);

    printf("[tid=%ld] Query status: %s\n", *tid, PQresStatus(resStatus));

    if (resStatus != PGRES_TUPLES_OK) {
        printf("[tid=%ld] Error executing the query: %s\n", *tid, PQerrorMessage(conn));
        PQclear(res);
        int err = db_conn_pool_push(p, conn);
        printf("[tid=%ld] Status of (early) connection release: %d\n", *tid, err);
        return NULL;
    }

    printf("[tid=%ld] Query executed successfully\n", *tid);

    int rows = PQntuples(res);
    int cols = PQnfields(res);
    printf("[tid=%ld] Number of rows: %d, number of columns: %d\n", *tid, rows, cols);

    for (int i = 0; i < cols; i++) {
        printf("[tid=%ld] %-15s", *tid, PQfname(res, i));
    }

    printf("\n");

    for (int i = 0; i < cols; i++) {
        printf("[tid=%ld] %-15d", *tid, PQftype(res, i));
    }

    printf("\n");

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("[tid=%ld] %-15s", *tid, PQgetvalue(res, i, j));
        }
        printf("\n");
    }

    printf("Before clear\n");

    PQclear(res);

    int err = db_conn_pool_push(p, conn);
    printf("[tid=%ld] Status of connection release: %d\n", *tid, err);
    return NULL;
}

int main(int argc, char** argv) {
    printf("Starting pool\n");

    char* conninfo = "dbname=db user=admin password=admin host=localhost port=5432";

    int db_pool_size = 4;
    p = db_conn_pool_create(db_pool_size, conninfo);

    printf("Pool created with %d connections\n", db_pool_size);

    int num_threads = 5;
    pthread_t tids[num_threads];

    for (int i = 0; i < num_threads; i++) {
        printf("%d\n", i);
        pthread_create(&tids[i], NULL, do_operation, (void*)(&i));
    }

    printf("Created %d threads. Waiting for execution\n", num_threads);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(tids[i], NULL);
    }

    printf("Joined threads\n");

    db_conn_pool_free(p);
    printf("Freed connection pool\n");
    return 0;
}