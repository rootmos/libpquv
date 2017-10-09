#!/bin/sh

DRIVER=${DRIVER-_build/test/driver}

PG_PORT=${PG_1_PORT_5432_TCP_PORT-5432}
PG_ADDR=${PG_1_PORT_5432_TCP_ADDR-127.0.0.1}

USER=postgres
PSQL="psql --user=$USER --host=$PG_ADDR --port=$PG_PORT"

wait_for_tcp() {
    echo "Testing $PG_ADDR:$PG_PORT..."

    if nc -z $PG_ADDR $PG_PORT; then
        echo "... connected"
    else
        sleep 2
        wait_for_tcp
    fi
}
wait_for_tcp

wait_for_pg() {
    echo "Checking pg..."

    if $PSQL --quiet --command "SELECT 1" > /dev/null; then
        echo "... ok"
    else
        sleep 2
        wait_for_pg
    fi
}
wait_for_pg

set -e

DB="db_$(cat /dev/urandom | tr -dc a-z0-9 | head -c 8)"
$PSQL --echo-queries --command="CREATE DATABASE $DB"

export PG_CONNINFO="postgresql://$USER@$PG_ADDR:$PG_PORT/$DB"
echo "url: $PG_CONNINFO"

PG_DUMMY_PORT=${SOCAT_PORT_5432_TCP_PORT-5432}
PG_DUMMY_ADDR=${SOCAT_PORT_5432_TCP_ADDR-127.0.0.1}
export PG_DUMMY_CONNINFO="postgresql://$USER@$PG_DUMMY_ADDR:$PG_DUMMY_PORT/$DB"

exec timeout ${TIMEOUT-2s} $DRIVER
