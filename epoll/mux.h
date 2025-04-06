#pragma once

#include "http.h"

typedef struct endpoint_s {
    struct endpoint_s* next;
    const char *method;
    const char *path;
    void *data;
    void (*handler)(http_req_t* req, int res, void *data);
} endpoint_t;

typedef struct mux_s {
    endpoint_t *endpoints;
} mux_t;

void mux_register_endpoint(mux_t *mux, endpoint_t *endpoint);

void mux_handler(http_req_t* req, int res_fd, void *data);
