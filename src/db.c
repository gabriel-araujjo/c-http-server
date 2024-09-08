#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "db.h"

int connection_equals(void* curr, void* conn) {
    return curr == conn;
}

void* db_conn_pool_connection_creator(void* args) {
    db_conn_pool* p = (db_conn_pool*)args;
    printf("Creating connection pool\n");
    int opened = 0;
    for (int i = 0; i < p->max_connections; i++) {
        PGconn* conn = PQconnectdb(p->conn_info);
        if (PQstatus(conn) != CONNECTION_OK) {
            printf("Error opening connection %d to server: %s\n", i, PQerrorMessage(conn));
            PQfinish(conn);
            continue;
        }
        opened++;
        db_conn_pool_push(p, conn);
    }
    if (!opened) {
        p->last_err = malloc(sizeof(db_conn_err));
        p->last_err->err_code = -1;
        p->last_err->err_msg = "Could not open connections to database";
        pthread_cond_signal(p->connections_available);
    }
    printf("[db_thread_pool] %d connections opened\n", opened);
    p->allocated = 1;
    return NULL;
}

db_conn_pool* db_conn_pool_create(int max_connections, char* conninfo) {
    db_conn_pool* p = malloc(sizeof(db_conn_pool));
    p->mtx = malloc(sizeof(pthread_mutex_t));
    p->connections_available = malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(p->mtx, NULL);
    pthread_cond_init(p->connections_available, NULL);
    p->connections = stack_create((data_free_fn)&PQfinish);

    p->max_connections = max_connections;
    p->empty = 1;
    p->allocated = 0;
    int len = sizeof(char) + strlen(conninfo);
    p->conn_info = malloc(len);
    snprintf(p->conn_info, len, "%s", conninfo);

    p->last_err = NULL;

    return p;
}

void db_conn_pool_free(db_conn_pool* p) {
    pthread_mutex_lock(p->mtx);

    stack_free(p->connections);
    free(p->conn_info);
    if (p->last_err) free(p->last_err);

    pthread_mutex_unlock(p->mtx);
    pthread_mutex_destroy(p->mtx);
    pthread_cond_destroy(p->connections_available);

    free(p);
    p = NULL;
}

PGconn* db_conn_pool_pop(db_conn_pool* p) {
    pthread_mutex_lock(p->mtx);

    if (p->empty || !p->allocated) {
        printf("Waiting available connnections on thread\n");
        if (!p->allocated) {
            pthread_t tid;
            pthread_create(&tid, NULL, db_conn_pool_connection_creator, (void*)p);
            pthread_detach(tid);
        }
        pthread_cond_wait(p->connections_available, p->mtx);
    }

    if (p->last_err) {
        return NULL;
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
    pthread_mutex_lock(p->mtx);

    int inserted = stack_push(p->connections, c);

    if (!inserted) {
        pthread_mutex_unlock(p->mtx);
        return 0;   
    }

    if (!p->allocated) p->allocated = 1;

    if (p->empty) {
        printf("Signaling that there's a free connection now %d\n", p->empty);
        p->empty = 0;
        pthread_cond_signal(p->connections_available);
    }

    pthread_mutex_unlock(p->mtx);
    return 1;
}
