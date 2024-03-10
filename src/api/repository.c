#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repository.h"
#include "utils.h"

client_repository_t* client_repository_create(db_conn_pool* p) {
    client_repository_t* r = malloc(sizeof(client_repository_t));
    r->pool = p;
    return r;
}

void client_repository_free(client_repository_t* r) {
    if (r) free(r);
    r = NULL;
}

void transaction_response_dto_free(void* ptr) {
    transaction_response_dto_t* t = (transaction_response_dto_t*)ptr;
    free(t);
}

void extract_response_dto_free(void* ptr) {
    extract_response_dto_t* e = (extract_response_dto_t*)ptr;
    if (e->saldo) {
        free(e->saldo->data_extrato);
        free(e->saldo);
    }

    for (int i = e->n_transacoes -1; i > 0; --i) {
        free(e->ultimas_transacoes[i]->tipo);
        e->ultimas_transacoes[i]->tipo = NULL;
        free(e->ultimas_transacoes[i]->descricao);
        e->ultimas_transacoes[i]->descricao = NULL;
        free(e->ultimas_transacoes[i]->realizada_em);
        e->ultimas_transacoes[i]->realizada_em = NULL;
        free(e->ultimas_transacoes[i]);
        e->ultimas_transacoes[i] = NULL;
    }

    if (e->ultimas_transacoes) free(e->ultimas_transacoes);
}

db_operation_result_t* client_repository_create_transaction(client_repository_t* r, transaction_dto_t* dto) {
    if (!r || !r->pool) return NULL;

    db_operation_result_t* response = db_operation_result_create(&transaction_response_dto_free);

    PGconn* conn = db_conn_pool_pop(r->pool);

    PGresult* res = PQexec(conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        char* err = PQerrorMessage(conn);
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        response->err = err;
        response->err_status = DB_GENERAL_ERROR;
        printf("Error opening transaction: %s\n", err);
        return response;
    }

    PQclear(res);

    char* id_str = itoa(*dto->id);
    const char* params[] = {id_str};
    res = PQexecParams(conn, "SELECT limite, saldo FROM clientes WHERE id=$1 FOR UPDATE", 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        char* err = PQerrorMessage(conn);
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        free(id_str);
        response->err = err;
        response->err_status = DB_GENERAL_ERROR;
        printf("Error executing select: %s\n", err);
        return response;
    }

    int rows = PQntuples(res);
    int cols = PQnfields(res);
    if (!rows || !cols) {
        PQclear(res);
        response->err = "Not found";
        response->err_status = DB_NOT_FOUND_ERROR;
        printf("Query didn't return any rows: %s\n", response->err);
        free(id_str);

        res = PQexec(conn, "END"); // END or ROLLBACK?
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            char* err = PQerrorMessage(conn);
            printf("Error ending transaction: %s\n", err);
        }
        PQclear(res);

        db_conn_pool_push(r->pool, conn);
        return response;
    }

    int limite = atoi(PQgetvalue(res, 0, 0));
    int saldo = atoi(PQgetvalue(res, 0, 1));
    int new_saldo = saldo - dto->valor;

    PQclear(res);

    if ((strcmp(dto->tipo, "d") == 0) && new_saldo < -limite) {
        response->err = "Unprocessable entity";
        response->err_status = DB_UNPROCESSABLE_ENTITY_ERROR;
        free(id_str);
        printf("Error updating balance: %s. Ending transaction\n", response->err);

        // res = PQexec(conn, "ROLLBACK"); // END or ROLLBACK?
        // if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        //     char* err = PQerrorMessage(conn);
        //     printf("Error rolling back transaction: %s\n", err);
        // }
        // PQclear(res);

        res = PQexec(conn, "END"); // END or ROLLBACK?
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            char* err = PQerrorMessage(conn);
            printf("Error ending transaction: %s\n", err);
        }

        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        return response;
    }

    char* new_saldo_str = itoa(new_saldo);
    const char* params2[] = {new_saldo_str, id_str};
    res = PQexecParams(conn, "UPDATE clientes SET saldo=$1 WHERE id=$2", 2, NULL, params2, NULL, NULL, 0); // or rollback?
    free(new_saldo_str);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        char* err = PQerrorMessage(conn);
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        free(id_str);
        response->err = err;
        response->err_status = DB_GENERAL_ERROR;
        printf("Error discarding transaction: %s\n", err);
        return response;
    }

    char* valor_str = itoa(dto->valor);
    const char* params3[] = {id_str, valor_str, dto->tipo, dto->descricao, dto->realizada_em};
    res = PQexecParams(conn, "INSERT INTO extrato(id_cliente, valor, tipo, descricao, realizada_em)"
    "VALUES($1, $2, $3, $4, $5)", 5, NULL, params3, NULL, NULL, 0);

    free(valor_str);
    free(id_str);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        char* err = PQerrorMessage(conn);
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        response->err = err;
        response->err_status = DB_GENERAL_ERROR;
        printf("Error updating extract: %s\n", err);
        return response;
    }

    PQclear(res);

    res = PQexec(conn, "COMMIT");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        char* err = PQerrorMessage(conn);
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        response->err = err;
        response->err_status = DB_GENERAL_ERROR;
        printf("Error commiting transaction: %s\n", err);
        return response;
    }

    PQclear(res);
    db_conn_pool_push(r->pool, conn);

    response->dto = malloc(sizeof(transaction_response_dto_t));
    ((transaction_response_dto_t*)response->dto)->limite = limite;
    ((transaction_response_dto_t*)response->dto)->saldo = new_saldo;
    return response;
}

db_operation_result_t* client_repository_get_extract(client_repository_t* r, int* id) {
    if (!r || !r->pool) return NULL;

    db_operation_result_t* response = db_operation_result_create(&extract_response_dto_free);

    PGconn* conn = db_conn_pool_pop(r->pool);

    char* id_str = itoa(*id);
    const char* params[] = {id_str};
    PGresult* res = PQexecParams(conn, "SELECT limite, saldo FROM clientes WHERE id=$1", 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        char* err = PQerrorMessage(conn);
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        free(id_str);
        response->err = err;
        response->err = DB_GENERAL_ERROR;
        printf("Error executing select for 'clientes': %s\n", err);
        return response;
    }

    int rows = PQntuples(res);
    int cols = PQnfields(res);
    if (!rows || !cols) {
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        free(id_str);
        response->err = "Not found";
        response->err_status = DB_NOT_FOUND_ERROR;
        printf("Query didn't return any rows: %s\n", response->err);
        return response;
    }

    int limite = atoi(PQgetvalue(res, 0, 0));
    int saldo = atoi(PQgetvalue(res, 0, 1));

    PQclear(res);

    const char* params2[] = {id_str};
    res = PQexecParams(conn, "SELECT valor, tipo, descricao, realizada_em FROM extrato WHERE id_cliente=$1 ORDER BY realizada_em DESC LIMIT 10", 1, NULL, params2, NULL, NULL, 0);
    free(id_str);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        char* err = PQerrorMessage(conn);
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        response->err = err;
        response->err_status = DB_GENERAL_ERROR;
        printf("Error executing select for 'extrato': %s\n", err);
        return response;
    }

    extract_response_dto_t* dto = malloc(sizeof(extract_response_dto_t));
    dto->saldo = malloc(sizeof(extract_saldo_dto_t));
    dto->ultimas_transacoes = NULL;

    dto->saldo->total = saldo;
    dto->saldo->limite = limite;
    dto->saldo->data_extrato = date_iso_now();

    response->dto = dto;

    rows = PQntuples(res);
    cols = PQnfields(res);
    if (!rows || !cols) {
        printf("No previous transactions were found in 'extrato' for this client; returning\n");
        PQclear(res);
        db_conn_pool_push(r->pool, conn);
        return response;
    }

    dto->ultimas_transacoes = malloc(sizeof(extract_dto_t*) * rows);
    for (int i = 0; i < rows; i++) {
        extract_dto_t* row = malloc(sizeof(extract_dto_t));
        row->valor = atoi(PQgetvalue(res, i, 0));
        char* val = PQgetvalue(res, i, 1);
        row->tipo = malloc(sizeof(char) + strlen(val));
        strncpy(row->tipo, val, sizeof(char) + strlen(val));
        val = PQgetvalue(res, i, 2);
        row->descricao = malloc(sizeof(char) + strlen(val));
        strncpy(row->descricao, val, sizeof(char) + strlen(val));
        val = PQgetvalue(res, i, 3);
        int date_len = strlen(val);
        row->realizada_em = malloc((2 * sizeof(char)) + date_len);
        strncpy(row->realizada_em, val, (2 * sizeof(char)) + date_len);
        row->realizada_em[10] = 'T';
        row->realizada_em[date_len] = 'Z';
        row->realizada_em[date_len+1] = '\0';
        dto->ultimas_transacoes[i] = row;
    }

    dto->n_transacoes = rows;

    PQclear(res);
    db_conn_pool_push(r->pool, conn);
    return response;
}