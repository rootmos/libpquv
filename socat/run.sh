#!/bin/sh
socat \
    TCP4-LISTEN:5432,bind=0.0.0.0,reuseaddr,fork \
    TCP4:$PG_1_PORT_5432_TCP_ADDR:$PG_1_PORT_5432_TCP_PORT
