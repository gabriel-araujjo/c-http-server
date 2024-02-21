#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// close include
#include <unistd.h>


#include <string.h>

#include <regex.h>

#include "http.h"
#include "generic_types.h"

//1 MB
#define BUFFER_SIZE 1048576

http_response* http_response_create(enum http_status_code status_code, char* body) {
    size_t s = sizeof(http_response);
    http_response* r = malloc(s);
    r->status_code = status_code;
    const char* status = NULL;
    switch(r->status_code) {
        case HTTP_STATUS_OK:
            status = "OK";
            break;
        case HTTP_STATUS_NOT_FOUND:
            status = "Not Found";
            break;
        case HTTP_STATUS_UNPROCESSABLE_ENTITY:
            status = "Unprocessable Entity";
            break;
        default:
            status = "Internal Server Error";
            break;
    }
    r->status = (char*)status;
    r->content_type = "application/json";
    r->body = body;
    return r;
}

http_response* http_response_ok(char* body) {
    return http_response_create(HTTP_STATUS_OK, body);
}

http_response* http_response_not_found() {
    return http_response_create(HTTP_STATUS_NOT_FOUND, NULL);
}

http_response* http_response_unprocessable_entity() {
    return http_response_create(HTTP_STATUS_UNPROCESSABLE_ENTITY, NULL);
}

http_response* http_response_internal_server_error() {
    return http_response_create(HTTP_STATUS_INTERNAL_SERVER_ERROR, NULL);
}

http_server* http_create_server(FILE* f, int port, int max_connections) {
    fprintf(f, "Allocating memory for server\n");
    http_server* s = malloc(sizeof(http_server));


    linked_list* e = linked_list_create(&free, NULL);
    s->endpoints = e;
    regcomp(&s->body_regex, "\r\n(\\{.*\\})", REG_EXTENDED);


    int server_fd;
    struct sockaddr_in server_addr;

    fprintf(f, "Creating server socket\n");
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(f, "Error opening server socket\n");
        return NULL;
    }

    // Fixes error on port reusing after server/application restart
    fprintf(f, "Setting reuse address option for socket\n");
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(f, "Error setting reuse address option for socket configuration\n");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    fprintf(f, "Binding port %d to socket\n", port);
    if (bind(server_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(f, "Error binding socket to the port\n");
        return NULL;
    }

    fprintf(f, "Listening to port for connections\n");
    if (listen(server_fd, max_connections) < 0) {
        fprintf(f, "Error listening for connections on socket\n");
        return NULL;
    }

    s->server_fd = server_fd;
    return s;
}

char* http_get_request(int client_fd) {
    char* buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    size_t bytes_received;

    if ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) < 0) {
        printf("Error reading request data\n");
        free(buffer);
        return NULL;
    }

    return buffer;
}

void http_send_response(int client_fd, http_response* r) {
    char* body = r->body != NULL ? r->body : "";

    char* output = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(output, BUFFER_SIZE, "HTTP/1.1 %d %s\r\n"
                "Content-Type: %s\r\n"
                 "\r\n%s", r->status_code, r->status, r->content_type, body);
    int resp_len = strlen(output);
    printf("response to send: %s\n", output);

    send(client_fd, output, resp_len, 0);
    free(output);
    close(client_fd);
}

regmatch_t* http_endpoint_matches_path(void* arg1, void* arg2) {
    http_endpoint* e = (http_endpoint*)arg1;
    char* path = (char*)arg2;

    regmatch_t* matches = malloc(sizeof(regmatch_t) * 2);

    if (regexec(e->regex, path, 2, matches, 0) == 0) {
        return matches;
    }

    free(matches);
    return NULL;
}

char* get_path_param(char* req_buff, regmatch_t* matches) {
    size_t path_param_s = matches[1].rm_eo - matches[1].rm_so + 1;
    char *path_param = malloc(path_param_s);
    memcpy(path_param, req_buff + matches[1].rm_so, path_param_s);
    path_param[matches[1].rm_eo] = '\0';
    return path_param;
}

char* get_body(regex_t* rgx, char* req_buff, regmatch_t* matches) {
    if (regexec(rgx, req_buff, 2, matches, 0) == 0) {
        size_t body_s = matches[1].rm_eo - matches[1].rm_so + 1;
        char* body = malloc(body_s);
        memcpy(body, req_buff + matches[1].rm_so, body_s);
        body[matches[1].rm_eo] = '\0';
        return body;
    }

    return NULL;
}

void *http_serve_request(void* args) {
    http_request_context* c = (http_request_context*) args;
    http_server* s = c->s;
    http_request* req = c->r;

    if (!s || !s->endpoints || !s->endpoints->current) return NULL;

    char* req_buff = http_get_request(req->client_fd);
    
    // Search the matcing route, if exists
    regmatch_t* matches;
    node* n = s->endpoints->current;
    for (; n != NULL; n = n->next) {
        matches = http_endpoint_matches_path((http_endpoint*)n->data, req_buff);
        if (matches) break;
    }

    // No route matched the path
    if (!matches) {
        http_response* resp = http_response_internal_server_error();
        http_send_response(req->client_fd, resp);
        return NULL;
    }

    http_endpoint* e = (http_endpoint*)n->data;

    // Get path param from request buffer
    char *path_param = get_path_param(req_buff, matches);
    char* req_body = get_body(&s->body_regex, req_buff, matches);
    int id = atoi(path_param);

    free(matches);

    // Handles request, sends response
    http_response* resp = e->handler(&id, req_body);
    http_send_response(req->client_fd, resp);
    return NULL;
}

void http_add_route(http_server* s, char* path, handler_fn handler) {
    http_endpoint* e = malloc(sizeof(http_endpoint));
    e->handler = handler;
    regex_t* regex = malloc(sizeof(regex_t));
    regcomp(regex, path, REG_EXTENDED);
    e->regex = regex;
    linked_list_add(s->endpoints, (void*)e);
    return;
}

void http_serve(FILE* f, http_server* s) {
    while (1) {
        int *client_fd = malloc(sizeof(int));
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        if ((*client_fd = accept(s->server_fd, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
            fprintf(f, "Accept failed\n");
            continue;
        }

        fprintf(f, "Receive request\n");

        http_request* r = malloc(sizeof(http_request));
        r->client_fd = *client_fd;
        http_request_context *c = malloc(sizeof(http_request_context));
        c->s = s;
        c->r = r;

        fprintf(f, "Creating thread to serve request\n");
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, http_serve_request, (void*)c);
        pthread_detach(thread_id);

        fprintf(f, "Thread created and detached to serve request\n");

        free(client_fd);

        fflush(f);
    }
    close(s->server_fd);
}