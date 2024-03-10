#include <stdlib.h>
#include <stdio.h>
#include "api_types.h"


db_operation_result_t* db_operation_result_create(dto_free_fn fn) {
    db_operation_result_t* r = malloc(sizeof(db_operation_result_t));
    r->err = NULL;
    r->dto = NULL;
    r->free_fn = fn;
    return r;
}

void db_operation_result_free(db_operation_result_t* r) {
    if (!r || !r->dto) return;
    if (r->dto) r->free_fn(r->dto);
    r->dto = NULL;
    free(r);
    r = NULL;
}

