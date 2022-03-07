#!/bin/sh

set -e

file="$1"
if [ -z "$file" ]; then
    echo "Usage: $0 FILE"
    exit 1
fi

sqlite3 symbls.db "
SELECT d.type, d.symbol, (COUNT(*) - (r.symbol IS NULL)) AS c
    FROM definition d LEFT JOIN reference r
        ON d.symbol=r.symbol AND d.type=r.type
    WHERE d.file='$file'
    GROUP BY d.type, d.symbol
    ORDER BY d.type, d.symbol
"
