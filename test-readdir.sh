#!/bin/sh
HOST="$1"
PORT="$2"

die() {
    echo -n "Error: $1"
    exit 1
}

if [ "$HOST" = "" -o "$PORT" = "" ]; then
    echo Usage: $0 host port
    exit 1
fi

./cfc $HOST $PORT null || die "cannot execute null operation"
./cfc $HOST $PORT rm /rdt 2>/dev/null
./cfc $HOST $PORT mkdir /rdt || die "cannot mkdir /rdt"
./cfc $HOST $PORT mkdir /rdt/foo || die "cannot mkdir /rdt/foo"
./cfc $HOST $PORT mkfile /rdt/bar || die "cannot mkfile /rdt/bar"
./cfc $HOST $PORT mkfile /rdt/foo/ABC || die "cannot mkfile /rdt/foo/ABC"
./cfc $HOST $PORT mkdir /rdt/quux || die "cannot mkdir /rdt/quux"
./cfc $HOST $PORT ls /rdt >/dev/null || die "cannot ls /rdt"
./cfc $HOST $PORT ls /rdt | grep -q rdt && die "ls /rdt contains rdt"
./cfc $HOST $PORT ls /rdt | grep -q foo || die "ls /rdt doesn't contain foo"
./cfc $HOST $PORT ls /rdt | grep -q foo || die "ls /rdt doesn't contain foo"
./cfc $HOST $PORT ls /rdt | grep -q bar || die "ls /rdt doesn't contain bar"
./cfc $HOST $PORT ls /rdt | grep -q ABC && die "ls /rdt contains ABC"
./cfc $HOST $PORT ls /rdt | grep -q quux || die "ls /rdt doesn't contain quux"
COUNT=`./cfc $HOST $PORT ls /rdt | wc -l`
if [ "$COUNT" != 3 ]; then
    die "ls /rdt returns $COUNT entries, should be 3"
fi

echo "Seems to work."
exit 0

