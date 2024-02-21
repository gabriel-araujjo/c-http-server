FROM gcc:4.9
COPY . /usr/src/app
WORKDIR /usr/src/app
RUN make
EXPOSE 8090
CMD ["./build/main"]