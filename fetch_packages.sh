#!/bin/sh

set -e

if [ -z "$DEBIAN_MIRROR" ]; then
    echo "DEBIAN_MIRROR not set"
    exit 1
fi

echo "Fetching Packages.xz"

curl -s -O "$DEBIAN_MIRROR/dists/Debian11.2/main/binary-amd64/Packages.xz"

xzcat Packages.xz | egrep '^(Package|Filename):' | xz -c >deb.xz

package="$1"
if [ -n "$package" ]; then
    awk_deps=$(cat <<EOF
/^Package:/ { r=\$2 }
/^Depends:.* $package / { print r }
EOF
)
    xzcat Packages.xz | awk "$awk_deps" | sort >"$package.rdeps"
fi

rm Packages.xz
