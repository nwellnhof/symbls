CREATE TABLE binary (
        path    TEXT PRIMARY KEY,
        soname  TEXT NOT NULL,
        package TEXT NOT NULL
) WITHOUT ROWID;
CREATE INDEX binary_package_index ON binary (package);

CREATE TABLE definition (
        path    TEXT NOT NULL,
        symbol  TEXT NOT NULL,
        type    CHAR(1) NOT NULL,
        bind    CHAR(1) NOT NULL,
        size    INTEGER NOT NULL,
        PRIMARY KEY (path, symbol)
) WITHOUT ROWID;
CREATE INDEX definition_symbol_index ON definition (symbol);

CREATE TABLE reference (
        path    TEXT NOT NULL,
        symbol  TEXT NOT NULL,
        type    CHAR(1) NOT NULL,
        PRIMARY KEY (path, symbol)
) WITHOUT ROWID;
CREATE INDEX reference_symbol_index ON reference (symbol);

CREATE TABLE so_reference (
        path    TEXT NOT NULL,
        soname  TEXT NOT NULL,
        PRIMARY KEY (path, soname)
) WITHOUT ROWID;
CREATE INDEX so_reference_soname_index ON so_reference (soname);

