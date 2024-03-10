# c-http-server
HTTP REST API written in C for networking studies 

## Context
This application was made to practice basic networking and data structures with C. Multithreading was also a point of interest. Please note that 
this code is not portable; Unix libraries are used throughout the application for 
multiple operations (mostly for pattern matching).

The challenge was to write a HTTP API using only a single external library for database access (`libpq`). JSON handling is manual.

A HTTP server thread pool and a database connection pool were also implemented.

## Building
You can use the `Dockerfile` for an isolated build:
```sh
make docker_build
```

Or you can run a native build. However, please note that you must first install `libpq`:
```sh
apt-get install libpq-dev
make build
```

- [Reference](https://www.postgresql.org/download/linux/ubuntu/)

To build and run, just execute `make run`. The server will listen for HTTP requests at port 8090.

## Improvements
Some improvements which can be cited are:

- Support for graceful exit would be cool, so database connections can be closed at server shutdown;
- A JSON handling library would improve the overall code and get rid with excessive C-style string operations;
- Stack allocation and cache-awareness: there's a ton of heap allocations throughout the code; a smarter use of stack variables could improve overall performance and decrese cache-misses during CPU instruction loading;
- Extension: the code is currently not extensible, and is deeply attached to the REST API definition (schema, inputs/outputs)
- Portability: just a single external library was necessary, but this code makes use of several Linux-native libraries;
- Benchmarking: currently, there are no benchmarks for the code;
- Documentation: details must be given about the chosen data structures and APIs

## References
Some useful references for this implementation are:

- https://man7.org/tlpi/
- https://www.postgresql.org/files/developer/concurrency.pdf
- https://www.postgresql.org/docs/current/tutorial-transactions.html
- https://www.postgresql.org/docs/current/libpq-example.html
