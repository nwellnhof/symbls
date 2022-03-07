#!/bin/sh

set -e

if [ -z "$DEBIAN_MIRROR" ]; then
    echo "DEBIAN_MIRROR not set"
    exit 1
fi

echo "Fetching Packages.xz"

curl -s -O "$DEBIAN_MIRROR/dists/Debian11.2/main/binary-amd64/Packages.xz"

xzcat Packages.xz | egrep '^(Package|Filename):' | xz -c >deb.xz

AWK_DEPS=$(cat <<'EOF'
/^Package:/ { p=$2 }
/^Depends:.* libxml2 / { print p }
EOF
)
xzcat Packages.xz | awk "$AWK_DEPS" | sort >libxml2.rdeps

rm Packages.xz
