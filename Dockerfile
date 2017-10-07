FROM debian:stretch-slim

RUN apt-get update && apt-get install --no-install-recommends -y \
  make cmake gcc libpq-dev libc-dev

RUN mkdir /libpquv
WORKDIR /libpquv

ADD Makefile .
ADD CMakeLists.txt .
ADD src src
ADD test test

RUN make test
