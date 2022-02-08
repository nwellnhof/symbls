# symbls

This is a simple, early-stage project that explores cross-references of
dynamic symbols in dynamic link libraries (DSOs, DLLs). It downloads
binary Debian packages and searches all contained ELF binaries for
definitions of or references to dynamic symbols. The data is stored in
an SQLite database.

My initial motivation was to get some insights into which open-source
projects happen to use symbols from libxml2, a very popular library
I currently maintain. Since libxml2 suffers from over-exporting many
internals, I was especially interested in symbols that none of the
600+ reverse dependencies use.

While Debian Linux was chosen for its vast number of packages, this is
intended to be cross-platform code. I actually started most of the
development on my MacBook.

## Build requirements and dependencies

- POSIX shell
- C compiler
- libelf
- sqlite3

## Overview

`fetch_packages.sh` fetches the `Packages.xz` file from a Debian
mirror, extracts a list of all packages depending on libxml2, and
creates a compressed textual database to lookup package URLs.

`update_package.sh` updates the database with the contents of a
package.

`update_rdeps.sh` updates the contents of all libxml2 reverse
dependencies. This downloads about 1 GB of packages, which are cached,
and creates a 450 MB SQLite database on my system.

`libxml2_syms.sh` generates an example report.

The `symbls` C program does most of the work extracting dynamic symbols
from the `.dynsym` section and gathering additional data.

## Future directions

This could be made into a useful web database, offering a
cross-reference of dynamic symbols in OSS C libraries.
