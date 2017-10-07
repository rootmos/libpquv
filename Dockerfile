FROM debian:stretch-slim

RUN mkdir -p /usr/share/man/man1 /usr/share/man/man7 \
  && apt-get update && apt-get install --no-install-recommends -y \
      make cmake gcc libpq-dev libc-dev netcat postgresql-client-9.6

RUN mkdir /libpquv
WORKDIR /libpquv

ADD Makefile .
ADD CMakeLists.txt .
ADD run-tests.sh .
ADD src src
ADD test test

RUN make

ENTRYPOINT ["./run-tests.sh"]
