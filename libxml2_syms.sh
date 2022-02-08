#!/bin/sh

sqlite3 symbls.db "
SELECT d.type, d.symbol, (COUNT(*) - (r.symbol IS NULL)) AS c
    FROM definition d LEFT JOIN reference r
        ON d.symbol=r.symbol AND d.type=r.type
    WHERE d.file='/usr/lib/x86_64-linux-gnu/libxml2.so.2.9.10'
    GROUP BY d.type, d.symbol
    ORDER BY d.type, d.symbol
"
