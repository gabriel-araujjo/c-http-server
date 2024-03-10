#ifndef repository_h
#define repository_h

#include "api_types.h"
#include "../db.h"

typedef struct client_repository_t {
    db_conn_pool* pool;
} client_repository_t;

client_repository_t* client_repository_create(db_conn_pool* p);
void client_repository_free(client_repository_t* r);
db_operation_result_t* client_repository_create_transaction(client_repository_t* r, transaction_dto_t* dto);
db_operation_result_t* client_repository_get_extract(client_repository_t* r, int* id);

#endif