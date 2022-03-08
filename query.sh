#!/bin/sh

set -e

command="$1"

case $command in

defs)
    path="$2"
    if [ -z "$path" ]; then
        echo "Usage: $0 defs BINARY-PATH"
        exit 1
    fi

    sqlite3 symbls.db "
SELECT d.type, d.symbol, d.bind, d.size, (COUNT(*) - (r.symbol IS NULL)) AS c
    FROM definition d LEFT JOIN reference r
        ON d.symbol=r.symbol
    WHERE d.path='$path'
    GROUP BY d.type, d.symbol
    ORDER BY d.type, d.symbol"
    ;;

package)
    package="$2"
    if [ -z "$package" ]; then
        echo "Usage: $0 package PACKAGE"
        exit 1
    fi

    sqlite3 symbls.db "
SELECT path, soname FROM binary WHERE package='$package'"
    ;;

refs)
    path="$2"
    if [ -z "$path" ]; then
        echo "Usage: $0 refs BINARY-PATH"
        exit 1
    fi

    sqlite3 symbls.db "
SELECT type, symbol FROM reference
    WHERE path='$path'
    ORDER BY type, symbol"
    ;;

symbol)
    symbol="$2"
    if [ -z "$symbol" ]; then
        echo "Usage: $0 symbol SYMBOL"
        exit 1
    fi

    echo "Definitions:"
    sqlite3 symbls.db "
    SELECT b.package, d.path, d.type FROM definition d
        LEFT JOIN binary b ON d.path=b.path
        WHERE d.symbol='$symbol'
        ORDER BY b.package, d.path, d.type"
    echo

    echo "References:"
    sqlite3 symbls.db "
SELECT b.package, r.path, r.type FROM reference r
    LEFT JOIN binary b ON r.path=b.path
    WHERE r.symbol='$symbol'
    ORDER BY b.package, r.path, r.type"
    ;;

*)
    echo "Usage: $0 (defs|package|refs|symbol) ..."
    exit 1
    ;;

esac
