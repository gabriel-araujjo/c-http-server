#ifndef db_h
#define db_h

#include <libpq-fe.h>
#include <pthread.h>

#include "generic_types.h"

typedef struct db_conn_err {
    char* err_msg;
    int err_code;
} db_conn_err;

/**
 * Thread-safe connection pool implementation for postgresql
*/
typedef struct db_conn_pool {
    stack* connections;
    int empty;
    int max_connections;
    int allocated;
    db_conn_err* last_err;
    char* conn_info;
    pthread_mutex_t* mtx;
    pthread_cond_t* connections_available;
} db_conn_pool;

db_conn_pool* db_conn_pool_create(int max_connections, char* conninfo);
void db_conn_pool_free(db_conn_pool* p);
PGconn* db_conn_pool_pop(db_conn_pool* p);
int db_conn_pool_push(db_conn_pool* p, PGconn* c);

int connection_equals(void* curr, void* conn);

#endif
