FROM gcc:4.9
COPY . /usr/src/app
WORKDIR /usr/src/app
RUN apt-get install libpq-dev
RUN make build
EXPOSE 8090
CMD ["./build/main", "2000", "400", "50"]