#!/bin/sh

set -e

file="$1"
if [ -z "$file" ]; then
    echo "Usage: $0 FILE"
    exit 1
fi

sqlite3 symbls.db "
SELECT type, symbol FROM reference
    WHERE file='$file'
    ORDER BY type, symbol
"
