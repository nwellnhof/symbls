#!/bin/sh

set -e

symbol="$1"
if [ -z "$symbol" ]; then
    echo "Usage: $0 SYMBOL"
    exit 1
fi

echo "Definitions:"
sqlite3 symbls.db "
SELECT b.package, d.path, d.type FROM definition d
    LEFT JOIN binary b ON d.path=b.path
    WHERE d.symbol='$symbol'
    ORDER BY b.package, d.path, d.type
"
echo

echo "References:"
sqlite3 symbls.db "
SELECT b.package, r.path, r.type FROM reference r
    LEFT JOIN binary b ON r.path=b.path
    WHERE r.symbol='$symbol'
    ORDER BY b.package, r.path, r.type
"
