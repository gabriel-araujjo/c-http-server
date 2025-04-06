#include "mux.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void mux_register_endpoint(mux_t *mux, endpoint_t *endpoint) {
    endpoint->next = mux->endpoints;
    mux->endpoints = endpoint;
}

void mux_handler(http_req_t* req, int res_fd, void *data)
{
    mux_t *mux = (mux_t *)data;

    // Handle the request
    endpoint_t *endpoint = mux->endpoints;
    printf("req: %s %s\n", req->method, req->path);
    while (endpoint != NULL) {
        printf("endpoint: %s %s\n", endpoint->method, endpoint->path);
        if (strcmp(endpoint->method, req->method) == 0 && strcmp(endpoint->path, req->path) == 0) {
            endpoint->handler(req, res_fd, endpoint->data);
            return;
        }
        endpoint = endpoint->next;
    }

    http_res_status(res_fd, 404, "Not Found");
    http_res_finish(res_fd);

    write(res_fd, "Not Found", strlen("Not Found"));
}
