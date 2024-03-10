#ifndef api_types_h
#define api_types_h

#include "../db.h"

enum db_error_status {
    DB_NOT_FOUND_ERROR,
    DB_UNPROCESSABLE_ENTITY_ERROR,
    DB_GENERAL_ERROR
};

typedef struct transaction_dto_t {
    int valor;
    char* tipo;
    char* descricao;
    char* realizada_em;
    int* id;
} transaction_dto_t;

typedef struct parse_result_t {
    transaction_dto_t* dto;
    char* err;
} parse_result_t;

typedef struct client_t {
    long limit;
    long balance;
} client_t;

typedef struct extract_t {
    long value;
    char* type;
    char* description;
    char* date;
} extract_t;

typedef struct extract_saldo_dto_t {
    int total;
    int limite;
    char* data_extrato;
} extract_saldo_dto_t;

typedef struct extract_dto_t {
    int valor;
    char* tipo;
    char* descricao;
    char* realizada_em;
} extract_dto_t;

typedef struct extract_response_dto_t {
    extract_saldo_dto_t* saldo;
    extract_dto_t** ultimas_transacoes;
    int n_transacoes;
} extract_response_dto_t;

typedef struct transaction_response_dto_t {
    int limite;
    int saldo;
} transaction_response_dto_t;

typedef void(*dto_free_fn)(void*);

typedef struct db_operation_result_t {
    void* dto;
    char* err;
    enum db_error_status err_status;
    dto_free_fn free_fn;
} db_operation_result_t;

db_operation_result_t* db_operation_result_create(dto_free_fn fn);
void db_operation_result_free(db_operation_result_t* r);

#endif