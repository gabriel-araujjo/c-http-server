#include "http.h"
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

static inline size_t next_token(char *str, size_t len, char c) {
  size_t i = 0;
  while (i < len && str[i] && str[i] != c)
    ++i;
  return i;
}

static inline char to_lower(char c) { return c & 0xdf; }

static inline int strcmp_i(const char *str1, const char *str2) {
  while (*str1 && *str2) {
    char c1 = to_lower(*str1);
    char c2 = to_lower(*str2);
    if (c1 != c2)
      return c1 - c2;
    ++str1;
    ++str2;
  }
  return to_lower(*str1) - to_lower(*str2);
}

const char *http_head_find(const http_head_t *headers, const char *name) {
  if (!name)
    return NULL;
  const http_head_t *current = headers;
  while (current->name != NULL) {
    if (!strcmp_i(current->name, name))
      return current->value;
    current++;
  }
  return NULL;
}

int http_req_parse(http_req_t *request, char *data, size_t len) {
  int i = 0;

  if (!request || !data)
    return 0;
  {
    if (i >= len)
      return 0;
    request->method = &data[i];
    i += next_token(&data[i], len - i, ' ');
    data[i] = '\0';
    ++i;
    printf("INFO: read method %s\n", request->method);

    if (i >= len)
      return 0;
    request->path = &data[i];
    i += next_token(&data[i], len - i, ' ');
    data[i] = '\0';
    ++i;
    printf("INFO: read path %s\n", request->path);

    if (i >= len)
      return 0;
    request->version = &data[i];
    i += next_token(&data[i], len - i, '\r');
    data[i] = '\0';
    ++i;
    printf("INFO: read version %s\n", request->version);
  }

  if (i >= len || data[i] != '\n')
    return 0;
  ++i;

  int j;
  for (j = 0; i < len && j <= MAX_HEADERS; ++j) {
    if (j == MAX_HEADERS)
      return 0;

    if (data[i] == '\r') {
      if (i + 1 >= len || !data[i + 1])
        return 0;
      i += 2;
      break;
    }

    if (i >= len)
      return 0;
    request->headers[j].name = &data[i];
    i += next_token(&data[i], len - i, ':');
    data[i] = '\0';
    ++i;
    while (data[i] == ' ')
      ++i;

    if (i >= len)
      return 0;
    request->headers[j].value = &data[i];
    i += next_token(&data[i], len - i, '\r');
    data[i] = '\0';
    ++i;

    printf("INFO: header %s: %s\n", request->headers[j].name,
           request->headers[j].value);

    if (i >= len || data[i] != '\n')
      return 0;

    ++i;
  }
  request->headers[j].name = NULL;
  request->headers[j].value = NULL;

  size_t body_len = len - i;
  if (body_len)
    request->body = &data[i];
  request->body_len = body_len;
  return 1;
}

int http_res_status(int fd, int status, const char *reason) {
  return dprintf(fd, "HTTP/1.1 %d %s\r\n", status, reason);
}

int http_res_header(int fd, const char *name, const char *value) {
  return dprintf(fd, "%s: %s\r\n", name, value);
}

int http_res_finish(int fd) { return write(fd, "\r\n", 2); }
