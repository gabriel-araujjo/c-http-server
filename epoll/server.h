#pragma once

#include "http.h"

typedef void (*server_handler)(http_req_t* req, int res_fd, void *data);

int http_listen(int port, server_handler handler, void *data);
