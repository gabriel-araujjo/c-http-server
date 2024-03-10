#ifndef controller_h
#define controller_h

#include <stdio.h>
#include <stdlib.h>

#include "api_types.h"
#include "repository.h"
#include "../http.h"

typedef struct client_service_t {
    client_repository_t* repository;
    handler_fn handler;
} client_service_t;

typedef struct extract_service_t {
    client_repository_t* repository;
    handler_fn handler;
} extract_service_t;

http_response* client_service_handler(void*, int* id, char* req);
client_service_t* client_service_create(client_repository_t* r);
http_response* extract_service_handler(void*, int* id, char* req);
extract_service_t* extract_service_create(client_repository_t* r);

char* transaction_response_dto_to_json(transaction_response_dto_t* dto);
char* extract_response_dto_to_json(extract_response_dto_t* dto);

void initialize_schemas();
void destroy_schemas();
char* get_string_match(char* buff, regmatch_t* matches);
parse_result_t* parse_and_validate(int* id, char* request);
int get_str_len_for_int(int val);
char* extract_dto_to_json(extract_dto_t** dto, int n);

#endif