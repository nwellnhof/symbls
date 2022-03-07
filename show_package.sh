#!/bin/sh

set -e

package="$1"
if [ -z "$package" ]; then
    echo "Usage: $0 FILE"
    exit 1
fi

sqlite3 symbls.db "
SELECT file, soname FROM file WHERE package='$package'
"
