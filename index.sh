#!/bin/sh

set -e

fetch_packages() {
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
/^(Pre-)?Depends:.* $package / { if (r) { print r } r="" }
EOF
    )
        xzcat Packages.xz | awk "$awk_deps" | sort >"$package.rdeps"
    fi

    rm Packages.xz
}

update_package() {
    package="$1"

    if [ ! -e deb.xz ]; then
        fetch_packages
    fi

    awk_find=$(cat <<'EOF'
$0 == "Package: " p { f++ }
f && /^Filename:/ { print $2; exit }
EOF
    )
    filename=$(xzcat deb.xz | awk -v "p=$package" "$awk_find")
    if [ -z "$filename" ]; then
        echo "$1: Package not found" 1>&2
        exit 1
    fi

    debfile="deb/$(basename "$filename")"
    if [ ! -e "$debfile" ]; then
        if [ -z "$DEBIAN_MIRROR" ]; then
            echo "DEBIAN_MIRROR not set"
            exit 1
        fi
        echo "Fetching $filename"
        mkdir -p deb
        curl -s -o "$debfile" "$DEBIAN_MIRROR/$filename"
    fi

    echo "Updating $package"

    rm -rf tmp
    mkdir -p tmp/data
    (cd tmp && ar -x "../$debfile")
    tar xf tmp/data.tar.[gx]z -C tmp/data

    ./symbls "$package" tmp/data
    retval=$?
    if [ $retval -eq 0 ]; then
        if [ ! -e symbls.db ]; then
            sqlite3 symbls.db <symbls.sql
        fi
        sqlite3 symbls.db <update_defs.sql >/dev/null
        sqlite3 symbls.db <update_refs.sql >/dev/null
    fi

    rm -f update_defs.sql update_refs.sql
    chmod -R u+w tmp
    rm -rf tmp
    return $retval
}

update_rdeps() {
    package="$1"

    if [ ! -e "$package.rdeps" ]; then
        fetch_packages "$package"
    fi

    update_package "$package"

    while read -r rdep; do
        update_package "$rdep"
    done <"$package".rdeps
}

command="$1"

case $command in
package)
    if [ -z "$2" ]; then
        echo "Usage: $0 package PACKAGE"
        exit 1
    fi
    update_package "$2"
    ;;
rdeps)
    if [ -z "$2" ]; then
        echo "Usage: $0 rdeps PACKAGE"
        exit 1
    fi
    update_rdeps "$2"
    ;;
*)
    echo "Usage: $0 (package|rdeps) PACKAGE"
    exit 1
    ;;
esac
