#!/bin/sh

set -e

symbol="$1"
if [ -z "$symbol" ]; then
    echo "Usage: $0 SYMBOL"
    exit 1
fi

echo "Definitions:"
sqlite3 symbls.db "
SELECT package, d.file FROM definition d
    LEFT JOIN file f ON d.file=f.file
    WHERE symbol='$symbol'
    ORDER BY package, d.file
"
echo

echo "References:"
sqlite3 symbls.db "
SELECT package, r.file FROM reference r
    LEFT JOIN file f ON r.file=f.file
    WHERE symbol='$symbol'
    ORDER BY package, r.file
"
