#include "http.h"
#include "mux.h"
#include "server.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>

void echo(http_req_t* req, int res, void *data) {
    http_res_status(res, 200, "OK");
    http_res_header(res, "Content-Type", "text/plain");
    http_res_finish(res);
    write(res, req->body, req->body_len);
}

int main() {
    endpoint_t echo_endpoint = {
        .method = "POST",
        .path = "/echo",
        .handler = echo
    };

    mux_t mux = {.endpoints = NULL };

    mux_register_endpoint(&mux, &echo_endpoint);

    http_listen(3030, mux_handler, &mux);

    return 0;
}
