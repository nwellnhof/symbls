#!/bin/sh

set -e

PACKAGE="$1"
if [ -z "$PACKAGE" ]; then
    echo "Usage: $0 PACKAGE"
    exit 1
fi

if [ ! -e deb.xz ]; then
    sh fetch_packages.sh
fi

AWK_FIND=$(cat <<'EOF'
$0 == "Package: " p { f++ }
f && /^Filename:/ { print $2; exit }
EOF
)
FILENAME=$(xzcat deb.xz | awk -v "p=$PACKAGE" "$AWK_FIND")
if [ -z "$FILENAME" ]; then
    echo "$1: Package not found" 1>&2
    exit 1
fi

DEBFILE="deb/$(basename "$FILENAME")"
if [ ! -e "$DEBFILE" ]; then
    if [ -z "$DEBIAN_MIRROR" ]; then
        echo "DEBIAN_MIRROR not set"
        exit 1
    fi
    echo "Fetching $FILENAME"
    mkdir -p deb
    curl -s -o "$DEBFILE" "$DEBIAN_MIRROR/$FILENAME"
fi

echo "Updating $PACKAGE"

rm -rf tmp
mkdir -p tmp/data
(cd tmp && ar -x "../$DEBFILE")
tar xf tmp/data.tar.xz -C tmp/data

./symbls "$PACKAGE" tmp/data
RETVAL=$?
if [ $RETVAL -eq 0 ]; then
    if [ ! -e symbls.db ]; then
        sqlite3 symbls.db <symbls.sql
    fi
    sqlite3 symbls.db <update_defs.sql
    sqlite3 symbls.db <update_refs.sql
fi

rm -f update_defs.sql update_refs.sql
rm -rf tmp
exit $RETVAL
