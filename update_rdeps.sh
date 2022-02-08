#!/bin/sh

if [ ! -e libxml2.rdeps ]; then
    sh fetch_packages.sh
fi

while read -r package; do
    sh update_package.sh "$package"
done <libxml2.rdeps
