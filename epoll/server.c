#include "server.h"
#include "http.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_CONNECTIONS 256
#define MAX_EVENTS 8

typedef struct client_s {
  int fd;
} client_t;

typedef struct clients_s {
  int i;
  uint bitmap[MAX_CONNECTIONS / sizeof(uint)];
  client_t clients[MAX_CONNECTIONS];
} clients_t;

static inline void client_free(clients_t *clients, uint client) {
  printf("INFO: freeing client at pos %d\n", client);
  client_t c = clients->clients[client];
  clients->bitmap[client / sizeof(uint)] &= ~(1 << (client % sizeof(uint)));
}

static inline uint client_alloc(clients_t *clients, int fd) {
  uint pos = clients->i;
  while (clients->bitmap[pos / sizeof(uint)]) {
    pos = (pos + 1) % MAX_CONNECTIONS;
  }

  printf("INFO: allocated client at pos %d\n", pos);

  clients->i = pos;
  clients->bitmap[pos / sizeof(uint)] |= 1 << (pos % sizeof(uint));
  clients->clients[pos] =
      (client_t){.fd = fd};

  return pos;
}

static inline client_t* clients_client(clients_t *clients, uint pos) {
  return &clients->clients[pos];
}

int http_listen(int port, server_handler handler, void *data) {
  struct sockaddr_in addr;
  int server, epollfd;
  clients_t clients;
  struct epoll_event ev, events[MAX_EVENTS];

  server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1)
    return -1;

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    printf("ERROR: fail to bind\n");
    close(server);
    return -1;
  }

  if (listen(server, MAX_CONNECTIONS) == -1) {
    printf("ERROR: fail to listen\n");
    close(server);
    return -1;
  }

  printf("INFO: listen on port %d\n", port);

  memset(&clients, 0, sizeof(clients_t));

  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    close(server);
    return -1;
  }

  ev.events = EPOLLIN;
  ev.data.fd = server;

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server, &ev) == -1) {
    close(epollfd);
    close(server);
    return -1;
  }

  for (;;) {
    int n = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (n == -1) {
        goto clean;
    }

    printf("INFO: read %d events\n", n);

    for (int i = 0; i < n; i++) {
      if (events[i].data.fd == server) {
        int fd = accept(server, NULL, 0);
        printf("INFO: accept new client\n");
        if (fd == -1) {
            goto clean;
        }
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            close(fd);
            goto clean;
        }
        flags |= O_NONBLOCK;
        fcntl(fd , F_SETFL , flags);
        uint client = client_alloc(&clients, fd);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.u64 = client;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            goto clean;
        }
      } else {
        uint pos = events[i].data.u32;
        client_t *c = clients_client(&clients, pos);

        char buf[4096];
        int n = read(c->fd, buf, sizeof(buf));

        if (n == -1) {
            goto clean_client;
        }

        http_head_t headers[MAX_HEADERS];
        http_req_t req = {
            .headers = headers,
        };

        printf("INFO: read %d bytes from client %d\n", n, pos);
        printf("%.*s\n\n", n, buf);

        if (!http_req_parse(&req, buf, n)) {
            printf("INFO: fail parsing request\n");
            goto clean_client;
        }

        handler(&req, c->fd, data);

        clean_client:
            printf("INFO: closing client %d\n", pos);
            if (epoll_ctl(epollfd, EPOLL_CTL_DEL, c->fd, NULL) == -1) {
                goto clean;
            }
            close(c->fd);
            client_free(&clients, pos);
      }
    }
  }

  clean:
  close(epollfd);
  close(server);

  return -1;
}
