CREATE TABLE file (
        file    TEXT NOT NULL,
        soname  TEXT NOT NULL,
        package TEXT NOT NULL
);

CREATE TABLE definition (
        file    TEXT NOT NULL,
        symbol  TEXT NOT NULL,
        type    CHAR(1) NOT NULL
);
CREATE INDEX definition_symbol_index ON definition (symbol);
CREATE INDEX definition_file_index ON definition (file);

CREATE TABLE reference (
        file    TEXT NOT NULL,
        symbol  TEXT NOT NULL,
        type    CHAR(1) NOT NULL
);
CREATE INDEX reference_symbol_index ON reference (symbol);
CREATE INDEX reference_file_index ON reference (file);

CREATE TABLE so_reference (
        file    TEXT NOT NULL,
        soname  TEXT NOT NULL
);
CREATE INDEX so_reference_soname_index ON so_reference (soname);
CREATE INDEX so_reference_file_index ON so_reference (file);

