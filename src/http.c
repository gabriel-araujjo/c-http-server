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

# define BUFFER_SIZE 8192

const char* JSON_BODY_BASIC_REGEX = "\r\n(\\{[\"a-zA-Z0-9:,_ ]{1,}\\}$)";

http_response* http_response_create(enum http_status_code status_code, char* body) {
    size_t s = sizeof(http_response);
    http_response* r = malloc(s);
    r->status_code = status_code;
    const char* status = NULL;
    switch(r->status_code) {
        case HTTP_STATUS_OK:
            status = "OK";
            break;
        case HTTP_STATUS_BAD_REQUEST:
            status = "Bad Request";
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
    r->body = body != NULL ? body : "";
    return r;
}

void http_response_free(http_response* r) {
    if (!r) return;
    if (strcmp(r->body, "") != 0) {
        free(r->body);
    }
    free(r);
    r = NULL;
}

http_response* http_response_ok(char* body) {
    return http_response_create(HTTP_STATUS_OK, body);
}

http_response* http_response_bad_request() {
    return http_response_create(HTTP_STATUS_BAD_REQUEST, NULL);
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

http_server* http_create_server(FILE* f, int port, int max_connections, int thread_pool_size) {
    fprintf(f, "Allocating memory for server\n");
    // JSON_BODY_BASIC_REGEX = malloc(sizeof(char) * 32);
    // strcpy(JSON_BODY_BASIC_REGEX, "\r\n(\\{[\"a-zA-Z0-9:,_ ]{1,}\\}$)");

    http_server* s = malloc(sizeof(http_server));
    s->body_regex = malloc(sizeof(regex_t));
    s->server_fd = malloc(sizeof(int));
    s->client_pool = http_client_pool_create(max_connections);
    s->thread_pool = malloc(sizeof(pthread_t) * thread_pool_size);
    s->thread_pool_size = malloc(sizeof(int));
    *s->thread_pool_size = thread_pool_size;
    s->port = malloc(sizeof(int));
    *s->port = port;

    s->endpoints = linked_list_create(&free, NULL);
    int result = regcomp(s->body_regex, JSON_BODY_BASIC_REGEX, REG_EXTENDED);
    char err[100];
    regerror(result, s->body_regex, &err, 100);

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

    *s->server_fd = server_fd;

    return s;
}

http_client_pool* http_client_pool_create(int max_size) {
    http_client_pool* p = malloc(sizeof(http_client_pool));
    p->q = queue_create(&free);
    p->size = malloc(sizeof(int));
    *p->size = 0;
    p->max_size = malloc(sizeof(int));
    *p->max_size = max_size;
    p->mtx = malloc(sizeof(pthread_mutex_t));
    p->is_available = malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(p->mtx, NULL);
    pthread_cond_init(p->is_available, NULL);
    return p;
}

void http_client_pool_free(http_client_pool* p) {
    if (!p) return;    
    pthread_mutex_lock(p->mtx);

    if (p->q) queue_free(p->q);
    if (p->size) free(p->size);
    if (p->max_size) free(p->max_size);

    pthread_mutex_unlock(p->mtx);
    pthread_mutex_destroy(p->mtx);
    pthread_cond_destroy(p->is_available);
}

int* http_client_pool_get(http_client_pool* p) {
    if (!p) return NULL;

    pthread_mutex_lock(p->mtx);

    if (queue_is_empty(p->q)) {
        pthread_cond_wait(p->is_available, p->mtx);
    }

    int* client_fd = NULL;

    if (!queue_is_empty(p->q)) {
        client_fd = queue_dequeue(p->q);
        *p->size--;
    }

    pthread_mutex_unlock(p->mtx);

    return client_fd;
}

int http_client_pool_add(http_client_pool* p, int* client_fd) {
    if (!p || *p->max_size <= *p->size) return 0;

    pthread_mutex_lock(p->mtx);

    queue_enqueue(p->q, client_fd);
    *p->size++;
    pthread_cond_broadcast(p->is_available);

    pthread_mutex_unlock(p->mtx);

    return 1;
}

char* http_get_request(int* client_fd) {
    char* buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    size_t bytes_received;

    if ((bytes_received = recv(*client_fd, buffer, BUFFER_SIZE, 0)) < 0) {
        printf("Error reading request data\n");
        free(buffer);
        return NULL;
    }

    char* new_buffer = realloc(buffer, bytes_received+1);
    new_buffer[bytes_received] = '\0';

    return new_buffer;
}

void http_send_response(int* client_fd, http_response* r) {
    int content_length = strlen(r->body);

    char* output = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(output, BUFFER_SIZE, "HTTP/1.1 %d %s\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %d\r\n"
                 "\r\n%s", r->status_code, r->status, r->content_type, content_length, r->body);
    int resp_len = strlen(output);

    int sent = send(*client_fd, output, resp_len, 0);
    if (sent == -1) {
        printf("Error sending response\n");
    }
    free(output);
    close(*client_fd);
    printf("Sent %d bytes to client\n", sent);
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
    size_t path_param_s = matches[1].rm_eo - matches[1].rm_so + sizeof(char);
    char *path_param = malloc(path_param_s);
    memcpy(path_param, req_buff + matches[1].rm_so, path_param_s - sizeof(char));
    path_param[matches[1].rm_eo - matches[1].rm_so] = '\0';
    return path_param;
}

char* get_body(regex_t* rgx, char* req_buff, regmatch_t* matches) {
    if (regexec(rgx, req_buff, 2, matches, 0) == 0) {
        size_t body_s = matches[1].rm_eo - matches[1].rm_so + sizeof(char);
        char* body = malloc(body_s);
        memcpy(body, req_buff + matches[1].rm_so, body_s-sizeof(char));
        body[matches[1].rm_eo - matches[1].rm_so] = '\0';
        return body;
    }

    return NULL;
}

// TODO: check if regex_t pointer is correctly freed on underlying struct 
// removal
void http_add_route(http_server* s, char* path, void* service, handler_fn handler) {
    http_endpoint* e = malloc(sizeof(http_endpoint));
    e->service = service;
    e->handler = handler;
    regex_t* regex = malloc(sizeof(regex_t));
    regcomp(regex, path, REG_EXTENDED);
    e->regex = regex;
    linked_list_add(s->endpoints, (void*)e);
    return;
}

void* http_serve_request(void* args) {
    http_server* s = (http_server*) args;
    http_client_pool* p = s->client_pool;

    while(1) {
        int* client_fd = http_client_pool_get(p);
        if (!client_fd) {
            continue;
        }

        printf("[thread=%p] received fd %d to process\n", pthread_self(), *client_fd);

        if (!s || !s->endpoints || !s->endpoints->current) return NULL;

        char* req_buff = http_get_request(client_fd);
        
        // Search the matching route, if exists
        regmatch_t* matches;
        node* n = s->endpoints->current;
        for (; n != NULL; n = n->next) {
            matches = http_endpoint_matches_path((http_endpoint*)n->data, req_buff);
            if (matches) break;
        }

        // No route matched the path
        if (!matches) {
            http_response* resp = http_response_internal_server_error();
            http_send_response(client_fd, resp);
            free(client_fd);
            free(resp);
            continue;
        }

        http_endpoint* e = (http_endpoint*)n->data;

        // Get path param from request buffer
        char *path_param = get_path_param(req_buff, matches);
        char* req_body = get_body(s->body_regex, req_buff, matches);
        int* id = malloc(sizeof(int));
        *id = atoi(path_param);

        free(matches);

        // Handles request, sends response
        http_response* resp = e->handler(e->service, id, req_body);
        http_send_response(client_fd, resp);
        free(client_fd);
        free(id);
        http_response_free(resp);
        free(req_body);
    }

    return NULL;
}

void http_serve(FILE* f, http_server* s) {
    pthread_t* tid = NULL;
    for (int i = 0; i < *s->thread_pool_size; i++) {
        tid = s->thread_pool++;
        pthread_create(tid, NULL, http_serve_request, (void*)s);
        pthread_detach(*tid);
    }

    int total = 0;

    printf("Server listening at port %d\n", *s->port);

    while (1) {
        int *client_fd = malloc(sizeof(int));
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        if ((*client_fd = accept(*s->server_fd, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
            printf("Accept failed\n");
            continue;
        }

        printf("[total=%03d] Received request with fd %d. Adding to queue\n", ++total, *client_fd);
        int status = http_client_pool_add(s->client_pool, client_fd);

        fflush(f);
    }
    close(*s->server_fd);
}