#!/bin/sh

DRIVER=${DRIVER-_build/test/driver}

PG_PORT=${PG_1_PORT_5432_TCP_PORT-5432}
PG_ADDR=${PG_1_PORT_5432_TCP_ADDR-127.0.0.1}

wait_for_pg() {
    echo "Testing $PG_ADDR:$PG_PORT..."

    if nc -z $PG_ADDR $PG_PORT; then
        echo "... connected"
    else
        sleep 2
        wait_for_pg
    fi
}

wait_for_pg

set -e

DB="db_$(cat /dev/urandom | tr -dc a-z0-9 | head -c 8)"
USER=postgres
psql --user=$USER --host=$PG_ADDR --port=$PG_PORT \
    --echo-queries \
    --command="CREATE DATABASE $DB"

export PG_CONNINFO="postgresql://$USER@$PG_ADDR:$PG_PORT/$DB"
echo "url: $PG_CONNINFO"
exec $DRIVER
