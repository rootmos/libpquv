FROM alpine:3.6

RUN apk update && apk add socat

ADD run.sh .

EXPOSE 5432

ENTRYPOINT ["./run.sh"]
