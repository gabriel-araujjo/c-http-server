#pragma once

#include <stddef.h>

#define MAX_HEADERS 256

typedef struct http_head {
  const char *name;
  const char *value;
} http_head_t;

typedef struct http_req {
  const char *method;
  const char *path;
  const char *version;
  http_head_t *headers;
  const char *body;
  size_t body_len;
} http_req_t;

/**
 * Parse an HTTP request from `data`.
 *
 * - `request` [out] The request to parse. `request->headers` must be and array with size `MAX_HEADERS`.
 * - `data` [in] The buffer containing the request.
 * - `len` The length of buffer.
 *
 * returns `1` on success, `0` on error.
 */
int http_req_parse(http_req_t *request, char *data, size_t len);

/**
 * Get the value of a header from `headers`.
 *
 * - `headers` [in] NULL terminated array of headers.
 * - `name` [in] The name of the header to search for.
 *
 * returns The value of the header, or `NULL` if not found.
 */
const char *http_head_find(const http_head_t *headers, const char *name);

int http_res_status(int fd, int status, const char *reason);
int http_res_header(int fd, const char *name, const char *value);
int http_res_finish(int fd);
