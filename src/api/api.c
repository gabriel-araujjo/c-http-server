#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <pthread.h>
#include <string.h>
#include "api.h"
#include "api_types.h"
#include "utils.h"

const char* TRANSACTION_DTO_VALOR_SCHEMA = "\"valor\":([0-9]{1,})";
const char* TRANSACTION_DTO_TIPO_SCHEMA = "\"tipo\":\"([c|d]{1})\"";
const char* TRANSACTION_DTO_DESCRICAO_SCHEMA = "\"descricao\":\"([a-zA-Z0-9 ]{1,10})\"";

regex_t* transaction_dto_valor = NULL;
regex_t* transaction_dto_tipo = NULL;
regex_t* transaction_dto_descricao = NULL;
pthread_mutex_t* mtx = NULL;

void initialize_schemas() {
    if (mtx) pthread_mutex_lock(mtx);
    else {
        mtx = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(mtx, NULL);
        pthread_mutex_lock(mtx);
    }

    if (transaction_dto_valor && transaction_dto_tipo && transaction_dto_descricao) return;

    transaction_dto_valor = malloc(sizeof(regex_t));
    transaction_dto_tipo = malloc(sizeof(regex_t));
    transaction_dto_descricao = malloc(sizeof(regex_t));

    regcomp(transaction_dto_valor, TRANSACTION_DTO_VALOR_SCHEMA, REG_EXTENDED);
    regcomp(transaction_dto_tipo, TRANSACTION_DTO_TIPO_SCHEMA, REG_EXTENDED);
    regcomp(transaction_dto_descricao, TRANSACTION_DTO_DESCRICAO_SCHEMA, REG_EXTENDED);

    if (mtx) pthread_mutex_unlock(mtx);
}

void destroy_schemas() {
    if (mtx) pthread_mutex_lock(mtx);

    if (transaction_dto_valor) regfree(transaction_dto_valor);
    if (transaction_dto_tipo) regfree(transaction_dto_tipo);
    if (transaction_dto_descricao) regfree(transaction_dto_descricao);   

    if (mtx) pthread_mutex_unlock(mtx);
}

char* get_string_match(char* buff, regmatch_t* matches) {
    size_t s = matches[1].rm_eo - matches[1].rm_so + sizeof(char);
    char* str = malloc(s);
    memcpy(str, buff + matches[1].rm_so, s - sizeof(char));
    str[matches[1].rm_eo - matches[1].rm_so] = '\0';
    return str;
}

parse_result_t* parse_and_validate(int* id, char* request) {
    parse_result_t* res = malloc(sizeof(parse_result_t));
    res->err = NULL;
    res->dto = NULL;

    regmatch_t* matches = malloc(sizeof(regmatch_t) * 2);


    if (regexec(transaction_dto_valor, request, 2, matches, 0) == 1) {
        res->err = "\"valor\" does not follow the required pattern";
        free(matches);
        return res;
    }


    char* valor = get_string_match(request, matches);

    if (regexec(transaction_dto_tipo, request, 2, matches, 0) == 1) {
        res->err = "\"tipo\" does not follow the required pattern";
        free(matches);
        return res;
    }

    char* tipo = get_string_match(request, matches);

    if (regexec(transaction_dto_descricao, request, 2, matches, 0) == 1) {
        res->err = "\"descricao\" does not follow the required pattern";
        free(matches);
        return res;
    }

    char* descricao = get_string_match(request, matches);

    transaction_dto_t* t = malloc(sizeof(transaction_dto_t));
    res->dto = t;
    t->descricao = descricao;
    t->tipo = tipo;
    t->valor = atoi(valor);
    t->realizada_em = date_iso_now();
    t->id = id;

    free(matches);
    return res;
}

int get_str_len_for_int(int val) {
    char* str = itoa(val);
    int len = strlen(str);
    free(str);
    return len;
}

char* transaction_response_dto_to_json(transaction_response_dto_t* dto) {
    int limite_len = get_str_len_for_int(dto->limite);
    int saldo_len = get_str_len_for_int(dto->saldo);
    char* template_1 = "{\"limite\":";
    char* template_2 = ",\"saldo\":";
    char* template_3 = "}";
    int total_len = strlen(template_1) + strlen(template_2) + strlen(template_3) + limite_len + saldo_len + sizeof(char);
    char* buff = malloc(total_len);
    snprintf(buff, total_len, "%s%d%s%d%s", template_1, dto->limite, template_2, dto->saldo, template_3);
    return buff;
}

char* extract_dto_to_json(extract_dto_t** dto, int n) {
    char* open_brackets = "[";
    int open_brackets_len = strlen(open_brackets);
    char* close_brackets = "]";
    int close_brackets_len = strlen(close_brackets);

    int template_len = sizeof(char) 
        + open_brackets_len 
        + close_brackets_len;

    // calling code expects a heap-allocated string, so let's respect that
    if (!dto) {
        char* buff = malloc(template_len);
        snprintf(buff, template_len, "%s%s", open_brackets, close_brackets);
        return buff;
    }

    char* field_1 = "{\"valor\":";
    char* field_2 = ",\"tipo\":\"";
    char* field_3 = "\",\"descricao\":\"";
    char* field_4 = "\",\"realizada_em\":\"";
    char* field_5 = "\"}";
    int field_1_len = strlen(field_1);
    int field_2_len = strlen(field_2);
    int field_3_len = strlen(field_3);
    int field_4_len = strlen(field_4);
    int field_5_len = strlen(field_5);
    
    char** arr = malloc(sizeof(char*));
    arr[0] = NULL;
    int total_elems = 0;

    for (int i = 0; i < n; i++) {
        int valor_len = get_str_len_for_int(dto[i]->valor);
        int tipo_len = strlen(dto[i]->tipo);
        int descricao_len = strlen(dto[i]->descricao);
        int realizada_em_len = strlen(dto[i]->realizada_em);

        char* prefix = i == 0 ? "" : ",";
        int json_len = strlen(prefix) 
            + valor_len 
            + tipo_len 
            + descricao_len 
            + realizada_em_len 
            + sizeof(char)
            + field_1_len
            + field_2_len
            + field_3_len
            + field_4_len
            + field_5_len;
        
        template_len += json_len - sizeof(char);
        char* json = malloc(json_len);

        snprintf(json, json_len, "%s%s%d%s%s%s%s%s%s%s", 
            prefix, 
            field_1, 
            dto[i]->valor, 
            field_2, 
            dto[i]->tipo, 
            field_3, 
            dto[i]->descricao, 
            field_4, 
            dto[i]->realizada_em, 
            field_5);

        arr = realloc(arr, ++total_elems * sizeof(char*));
        arr[total_elems-1] = json;
    }

    char* buff = malloc(template_len);

    // this function always null-terminates non-empty string buffers, 
    // so we add another character to MAXLEN to guarantee copying
    // maybe, we would be better off with strcpy in this scenario
    // snprintf(buff, open_brackets_len + sizeof(char), "%s", open_brackets);
    strcpy(buff, open_brackets);
    int bytes_copied = open_brackets_len;

    for (int i = 0; i < total_elems; i++) {
        int json_len = strlen(arr[i]) + sizeof(char);
        snprintf(buff + (bytes_copied * sizeof(char)), json_len, "%s", arr[i]);
        bytes_copied += json_len - sizeof(char);
        free(arr[i]);
    }

    free(arr);

    strcpy(buff + (bytes_copied * sizeof(char)), close_brackets);
    return buff;
}

char* extract_response_dto_to_json(extract_response_dto_t* dto) {
    char* field_1 = "{\"total\":";
    char* field_2 = ",\"limite\":";
    char* field_3 = ",\"data_extrato\":\"";
    char* field_4 = "\"}";
    int field_1_len = strlen(field_1);
    int field_2_len = strlen(field_2);
    int field_3_len = strlen(field_3);
    int field_4_len = strlen(field_4);
    
    int total_str_len = get_str_len_for_int(dto->saldo->total);
    int limite_str_len = get_str_len_for_int(dto->saldo->limite);

    int saldo_field_len = sizeof(char)
        + strlen(dto->saldo->data_extrato)
        + total_str_len
        + limite_str_len
        + field_1_len
        + field_2_len
        + field_3_len
        + field_4_len;
    
    char* saldo = malloc(saldo_field_len);
    snprintf(saldo, saldo_field_len, "%s%d%s%d%s%s%s", field_1, dto->saldo->total, field_2, dto->saldo->limite, field_3, dto->saldo->data_extrato, field_4);
    
    char* extract = extract_dto_to_json(dto->ultimas_transacoes, dto->n_transacoes);
    int extract_len = strlen(extract);

    // parent nodes
    char* field_5 = "{\"saldo\":";
    char* field_6 = ",\"ultimas_transacoes\":";
    char* field_7 = "}";
    int field_5_len = strlen(field_5);
    int field_6_len = strlen(field_6);
    int field_7_len = strlen(field_7);

    // no need to add sizeof(char) for the null-terminating char, because
    // 'saldo_field_len' already includes that
    int total_len = field_5_len 
        + field_6_len
        + field_7_len
        + saldo_field_len
        + extract_len;
    char* buff = malloc(total_len);

    snprintf(buff, total_len, "%s%s%s%s%s", field_5, saldo, field_6, extract, field_7);

    free(saldo);
    free(extract);

    return buff;
}

http_response* create_error_response(db_operation_result_t* result) {
    switch(result->err_status) {
        case DB_NOT_FOUND_ERROR:
            return http_response_not_found();
            break;
        case DB_UNPROCESSABLE_ENTITY_ERROR:
            return http_response_unprocessable_entity();
            break;
        default:
            return http_response_internal_server_error();
            break;
    }
}

http_response* client_service_handler(void* c, int* id, char* req) {
    client_service_t* service = (client_service_t*)c;

    parse_result_t* result = parse_and_validate(id, req);
    if (result->err) {
        printf("client - error parsing and validating request: %s\n", result->err);
        return http_response_unprocessable_entity();
    }

    db_operation_result_t* res = client_repository_create_transaction(service->repository, result->dto);
    if (res->err) {
        printf("client - error creating transaction: %s\n", res->err);
        http_response* response = create_error_response(res);
        db_operation_result_free(res);
        return response;
    }

    http_response* response = http_response_ok(transaction_response_dto_to_json((transaction_response_dto_t*)res->dto));
    db_operation_result_free(res);
    return response;
}

client_service_t* client_service_create(client_repository_t* r) {
    client_service_t* c = (client_service_t*)malloc(sizeof(client_service_t));
    c->repository = r;
    c->handler = client_service_handler;
    initialize_schemas();
    return c;
}

http_response* extract_service_handler(void* c, int* id, char* req) {
    extract_service_t* service = (extract_service_t*)c;

    db_operation_result_t* res = client_repository_get_extract(service->repository, id);
    if (res->err) {
        printf("extract - error getting extract: %s\n", res->err);
        http_response* response = create_error_response(res);
        db_operation_result_free(res);
        return response;
    }

    http_response* response = http_response_ok(extract_response_dto_to_json((extract_response_dto_t*)res->dto));
    db_operation_result_free(res);
    return response;
}

extract_service_t* extract_service_create(client_repository_t* r) {
    extract_service_t* e = (extract_service_t*)malloc(sizeof(extract_service_t));
    e->repository = r;
    e->handler = extract_service_handler;
    return e;
}