FROM alpine:latest

RUN apk add --no-cache make gcc libc-dev gcc libc-dev python3 py3-pip ruby bash

COPY ./src/ ./src/
COPY ./Makefile ./Makefile
COPY ./benchmark.sh ./benchmark.sh

ENTRYPOINT [ "./benchmark.sh" ]