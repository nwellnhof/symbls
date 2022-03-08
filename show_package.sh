#!/bin/sh

set -e

package="$1"
if [ -z "$package" ]; then
    echo "Usage: $0 PACKAGE"
    exit 1
fi

sqlite3 symbls.db "
SELECT path, soname FROM binary WHERE package='$package'
"
