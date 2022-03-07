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

`show_sym.sh` shows information about a symbol.

The `symbls` C program does most of the work extracting dynamic symbols
from the `.dynsym` section and gathering additional data.

## Example

Create or update the database. This will take a while.

    $ sh update_rdeps.sh libxslt1.1

Show information about the package.

    $ sh show_package.sh libxslt1.1
    /usr/lib/x86_64-linux-gnu/libxslt.so.1.1.34|libxslt.so.1
    /usr/lib/x86_64-linux-gnu/libexslt.so.0.8.20|libexslt.so.0

Show exported symbols.

```
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

Show information about a symbol.

```
$ sh show_sym.sh xsltApplyStylesheetUser
Definitions:
libxslt1.1|/usr/lib/x86_64-linux-gnu/libxslt.so.1.1.34

References:
chromium|/usr/lib/chromium/chromium
chromium-shell|/usr/lib/chromium/chromium-shell
libapache2-mod-parser3|/usr/lib/apache2/modules/mod_parser3.so
libnetcf1|/usr/lib/x86_64-linux-gnu/libnetcf.so.1.4.0
libnginx-mod-http-xslt-filter|/usr/lib/nginx/modules/ngx_http_xslt_filter_module.so
libosinfo-1.0-0|/usr/lib/x86_64-linux-gnu/libosinfo-1.0.so.0.1008.0
libqt5webenginecore5|/usr/lib/x86_64-linux-gnu/libQt5WebEngineCore.so.5.15.2
libqt5webkit5|/usr/lib/x86_64-linux-gnu/libQt5WebKit.so.5.212.0
libraptor2-0|/usr/lib/x86_64-linux-gnu/libraptor2.so.0.0.0
libreoffice-core|/usr/lib/libreoffice/program/libucpchelp1.so
libreoffice-core|/usr/lib/libreoffice/program/libxsltfilterlo.so
libreoffice-core-nogui|/usr/lib/libreoffice/program/libucpchelp1.so
libreoffice-core-nogui|/usr/lib/libreoffice/program/libxsltfilterlo.so
libwebkit2gtk-4.0-37|/usr/lib/x86_64-linux-gnu/libwebkit2gtk-4.0.so.37.55.4
libwpewebkit-1.0-3|/usr/lib/x86_64-linux-gnu/libWPEWebKit-1.0.so.3.16.3
libxml-libxslt-perl|/usr/lib/x86_64-linux-gnu/perl5/5.32/auto/XML/LibXSLT/LibXSLT.so
libxmlsec1|/usr/lib/x86_64-linux-gnu/libxmlsec1.so.1.2.31
libyelp0|/usr/lib/x86_64-linux-gnu/libyelp.so.0.0.0
parser3-cgi|/usr/bin/parser3
php7.4-xml|/usr/lib/php/20190902/xsl.so
postgresql-13|/usr/lib/postgresql/13/lib/pgxml.so
python3-lxml|/usr/lib/python3/dist-packages/lxml/etree.cpython-39-x86_64-linux-gnu.so
python3-lxml-dbg|/usr/lib/python3/dist-packages/lxml/etree.cpython-39d-x86_64-linux-gnu.so
tclxml|/usr/lib/tcltk/x86_64-linux-gnu/tclxml3.2/libtclxml3.2.so
xmlstarlet|/usr/bin/xmlstarlet
xsltproc|/usr/bin/xsltproc
```

## Future directions

This could be made into a useful web database, offering a
cross-reference of dynamic symbols in OSS C libraries.
