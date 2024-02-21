#ifndef controller_h
#define controller_h

#include <stdio.h>
#include <stdlib.h>

#include "http.h"

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

typedef struct client_repository {

} client_repository;

typedef struct extract_repository {

} extract_repository;

typedef struct client_service {
    client_repository* repository;
    handler_fn handler;
} client_service;

http_response* client_service_handler(int* id, char* req) {
    printf("request received on handler id: %d, req: %s\n", *id, req);
    return http_response_ok("{\"limite\":100000,\"saldo\":-9098}");
}

client_service* client_service_create() {
    client_service* c = (client_service*)malloc(sizeof(client_service));
    c->handler = client_service_handler;
    return c;
}

#endif