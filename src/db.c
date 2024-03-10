#include <stdio.h>
#include <stdlib.h>
#include "db.h"

int connection_equals(void* curr, void* conn) {
    return curr == conn;
}

db_conn_pool* db_conn_pool_create(int max_connections, char* conninfo) {
    db_conn_pool* p = malloc(sizeof(db_conn_pool));
    p->mtx = malloc(sizeof(pthread_mutex_t));
    p->connections_available = malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(p->mtx, NULL);
    pthread_cond_init(p->connections_available, NULL);
    p->connections = stack_create((void*)(void*)&PQfinish);

    for (int i = 0; i < max_connections; i++) {
        PGconn* conn = PQconnectdb(conninfo);
        if (PQstatus(conn) != CONNECTION_OK) {
            printf("Error opening %d connection to server: %s\n", i, PQerrorMessage(conn));
            PQfinish(conn);
        }
        stack_push(p->connections, conn);
    }
    printf("[db_thread_pool] %d connections opened\n", max_connections);

    p->empty = 0;

    return p;
}

void db_conn_pool_free(db_conn_pool* p) {
    pthread_mutex_lock(p->mtx);

    stack_free(p->connections);

    pthread_mutex_unlock(p->mtx);
    pthread_mutex_destroy(p->mtx);
    pthread_cond_destroy(p->connections_available);

    free(p);
    p = NULL;
}

PGconn* db_conn_pool_pop(db_conn_pool* p) {
    pthread_mutex_lock(p->mtx);

    if (p->empty) {
        printf("Waiting available connnections on thread\n");
        pthread_cond_wait(p->connections_available, p->mtx);
    }

    PGconn* conn = NULL;

    if (!p->empty) {
        conn = stack_pop(p->connections);
    }

    if (!p->connections->head) {
        printf("Setting as empty\n");
        p->empty = 1;
    }

    pthread_mutex_unlock(p->mtx);
    return conn;
}

int db_conn_pool_push(db_conn_pool* p, PGconn* c) {
    // printf("conn %ld\n", c);
    pthread_mutex_lock(p->mtx);

    int inserted = stack_push(p->connections, c);

    if (!inserted) {
        pthread_mutex_unlock(p->mtx);
        return 0;   
    }

    if (p->empty) {
        printf("Signaling that there's a free connection now %d\n", p->empty);
        p->empty = 0;
        pthread_cond_signal(p->connections_available);
    }

    pthread_mutex_unlock(p->mtx);
    return 1;
}
