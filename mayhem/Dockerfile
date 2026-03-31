FROM debian:bookworm as builder

RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install build-essential clang -y

ADD . /project
WORKDIR /project/fuzz

RUN make

FROM debian:bookworm
COPY --from=builder /project/fuzz/qoiview-fuzz /