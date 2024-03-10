#ifndef http_h
#define http_h

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <pthread.h>

#include "generic_types.h"
#include "queue.h"

enum http_status_code {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_UNPROCESSABLE_ENTITY = 422,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
};

typedef struct http_response {
    enum http_status_code status_code;
    char* status;
    char* content_type;
    char* body;
} http_response;

typedef http_response* (*handler_fn)(void*, int*, char*);

typedef struct http_endpoint {
    regex_t* regex;
    void* service;
    handler_fn handler;
} http_endpoint;

typedef struct http_client_pool {
    queue* q;
    int* size;
    int* max_size;
    pthread_mutex_t* mtx;
    pthread_cond_t* is_available;
} http_client_pool;

typedef struct http_server {
    // File descriptor for the server socket
    int* server_fd;
    linked_list* endpoints;
    regex_t* body_regex;
    http_client_pool* client_pool;
    pthread_t* thread_pool;
    int* thread_pool_size;
    int* port;
} http_server;

typedef struct http_request {
    int client_fd;
} http_request;

typedef struct http_request_context {
    http_server* s;
    http_request* r;
} http_request_context;

http_response* http_response_create(enum http_status_code status_code, char* body);
void http_response_free(http_response* r);
http_response* http_response_ok(char* body);
http_response* http_response_not_found();
http_response* http_response_unprocessable_entity();
http_response* http_response_internal_server_error();
http_server* http_create_server(FILE* f, int port, int max_connections, int thread_pool_size);
http_client_pool* http_client_pool_create(int max_size);
void http_client_pool_free(http_client_pool* p);
int* http_client_pool_get(http_client_pool* p);
int http_client_pool_add(http_client_pool* p, int* client_fd);
char* http_get_request(int* client_fd);
void http_send_response(int* client_fd, http_response* r);
regmatch_t* http_endpoint_matches_path(void* arg1, void* arg2);
char* get_path_param(char* req_buff, regmatch_t* matches);
char* get_body(regex_t* rgx, char* req_buff, regmatch_t* matches);
void *http_serve_request(void* args);
void http_add_route(http_server* s, char* path, void* service, handler_fn handler);
void http_serve(FILE* f, http_server* s);

#endif