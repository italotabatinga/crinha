FROM alpine:latest AS builder

RUN apk add make gcc libc-dev

WORKDIR /app

COPY src/ ./src/
COPY Makefile ./

RUN make clean && make

FROM alpine:latest

COPY --from=builder /app/build/main /crinha

ENTRYPOINT [ "/crinha" ]
CMD ["/var/rinha/source.rinha"]