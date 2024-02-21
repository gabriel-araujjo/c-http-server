#ifndef http_h
#define http_h

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>

#include "generic_types.h"

enum http_status_code {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_UNPROCESSABLE_ENTITY = 422,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
};

// const char* http_status_msgs[] = {"OK", "Not Found", "Unprocessable Entity", "Internal Server Error"};

typedef struct http_response {
    enum http_status_code status_code;
    char* status;
    char* content_type;
    char* body;
} http_response;

typedef http_response* (*handler_fn)(int*, char*);

typedef struct http_endpoint {
    regex_t* regex;
    handler_fn handler;
} http_endpoint;

typedef struct http_server {
    // File descriptor for the server socket
    int server_fd;
    linked_list* endpoints;
    regex_t body_regex;
} http_server;

typedef struct http_request {
    int client_fd;
} http_request;

typedef struct http_request_context {
    http_server* s;
    http_request* r;
} http_request_context;

http_response* http_response_create(enum http_status_code status_code, char* body);
http_response* http_response_ok(char* body);
http_response* http_response_not_found();
http_response* http_response_unprocessable_entity();
http_response* http_response_internal_server_error();
http_server* http_create_server(FILE* f, int port, int max_connections) ;
char* http_get_request(int client_fd);
void http_send_response(int client_fd, http_response* r);
regmatch_t* http_endpoint_matches_path(void* arg1, void* arg2);
char* get_path_param(char* req_buff, regmatch_t* matches);
char* get_body(regex_t* rgx, char* req_buff, regmatch_t* matches);
void *http_serve_request(void* args);
void http_add_route(http_server* s, char* path, handler_fn handler);
void http_serve(FILE* f, http_server* s);

#endif