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
mirror and extracts useful data.

`update_package.sh` updates the database with the contents of a
package.

`update_rdeps.sh` updates the database with all reverse dependencies
of a package.

`show_package.sh` shows information about the binaries found in a
package.

`show_refs.sh` shows use counts for each exported symbol in a binary.

The `symbls` C program does most of the work extracting dynamic symbols
from the `.dynsym` section and gathering additional data.

## Example

```sh
$ sh update_rdeps.sh libxslt1.1
$ sh show_package.sh libxslt1.1
/usr/lib/x86_64-linux-gnu/libxslt.so.1.1.34|libxslt.so.1
/usr/lib/x86_64-linux-gnu/libexslt.so.0.8.20|libexslt.so.0
$ sh show_refs.sh /usr/lib/x86_64-linux-gnu/libxslt.so.1.1.34
F|xslAddCall|0
F|xslDropCall|0
F|xslHandleDebugger|0
F|xsltAddKey|0
F|xsltAddStackElemList|0
F|xsltAddTemplate|0
F|xsltAllocateExtra|0
F|xsltAllocateExtraCtxt|0
F|xsltApplyAttributeSet|0
F|xsltApplyImports|0
F|xsltApplyOneTemplate|4
F|xsltApplyStripSpaces|0
F|xsltApplyStylesheet|51
F|xsltApplyStylesheetUser|24
F|xsltApplyTemplates|0
...
```

## Future directions

This could be made into a useful web database, offering a
cross-reference of dynamic symbols in OSS C libraries.
