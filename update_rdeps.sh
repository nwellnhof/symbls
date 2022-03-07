#!/bin/sh

set -e

package="$1"
if [ -z "$package" ]; then
    echo "Usage: $0 PACKAGE"
    exit 1
fi

if [ ! -e "$package.rdeps" ]; then
    sh fetch_packages.sh "$package"
fi

sh update_package.sh "$package"

while read -r rdep; do
    sh update_package.sh "$rdep"
done <"$package".rdeps
