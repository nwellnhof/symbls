#!/bin/sh

set -e

path="$1"
if [ -z "$path" ]; then
    echo "Usage: $0 BINARY-PATH"
    exit 1
fi

sqlite3 symbls.db "
SELECT type, symbol FROM reference
    WHERE path='$path'
    ORDER BY type, symbol
"
