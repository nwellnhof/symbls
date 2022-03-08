#!/bin/sh

set -e

path="$1"
if [ -z "$path" ]; then
    echo "Usage: $0 BINARY-PATH"
    exit 1
fi

sqlite3 symbls.db "
SELECT d.type, d.symbol, d.bind, d.size, (COUNT(*) - (r.symbol IS NULL)) AS c
    FROM definition d LEFT JOIN reference r
        ON d.symbol=r.symbol
    WHERE d.path='$path'
    GROUP BY d.type, d.symbol
    ORDER BY d.type, d.symbol
"
