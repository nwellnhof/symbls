#define _XOPEN_SOURCE 500

#include <fcntl.h>
#include <ftw.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libelf.h>
#include <gelf.h>

#ifndef R_X86_64_COPY
  #define R_X86_64_COPY 5
#endif
#ifndef STT_GNU_IFUNC
  #define STT_GNU_IFUNC 10
#endif
#ifndef STB_GNU_UNIQUE
  #define STB_GNU_UNIQUE 10
#endif

static const char *g_package;
static const char *g_data_dir;
static size_t g_len_data_dir;
static FILE *g_out_def;
static FILE *g_out_ref;

static void
elf_perror(const char *s) {
    fprintf(stderr, "%s: %s\n", s, elf_errmsg(elf_errno()));
}

static int
find_copy_relocs(Elf *elf, size_t s_ndx, unsigned *syms) {
    int k = 0;

    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr_mem;
        GElf_Shdr *shdr = gelf_getshdr(scn, &shdr_mem);
        if (shdr == NULL)
            return -1;

        if (shdr->sh_type == SHT_REL && shdr->sh_link == s_ndx) {
            /* The valgrind package has SHT_REL! */
            fprintf(stderr, "warning: SHT_REL found\n");
        } else if (shdr->sh_type == SHT_RELA && shdr->sh_link == s_ndx) {
            Elf_Data *rela_data = elf_getdata(scn, NULL);
            if (rela_data == NULL)
                return -1;

            size_t nentries = shdr->sh_entsize ?
                              shdr->sh_size / shdr->sh_entsize :
                              0;
            for (size_t cnt = 0; cnt < nentries; ++cnt) {
                GElf_Rela relmem;
                GElf_Rela *rel = gelf_getrela(rela_data, cnt, &relmem);
                if (rel == NULL)
                    return -1;

                if (GELF_R_TYPE(rel->r_info) != R_X86_64_COPY)
                    continue;
                if (syms)
                    syms[k] = GELF_R_SYM(rel->r_info);
                k++;
            }
        }
    }

    return k;
}

static void
warn(const char *path, int *flag, const char *fmt, ...) {
    va_list ap;

    if (*flag == 0) {
        fprintf(stderr, "Warnings for %s\n", path);
        *flag = 1;
    }

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static int
process_file(const char *filename, const struct stat *sb, int typeflag,
             struct FTW *info) {
    (void) sb;
    (void) info;
    int ret = 1;
    int fd = -1;
    Elf *elf = NULL;
    unsigned *relocs = NULL;
    int is_lib;
    int warn_flag = 0;

    if (typeflag != FTW_F)
        return 0;

    if (strncmp(filename, g_data_dir, g_len_data_dir) != 0 ||
        filename[g_len_data_dir] != '/') {
        fprintf(stderr, "%s: doesn't start with '%s/'", filename, g_data_dir);
        goto error;
    }
    const char *path = filename + g_len_data_dir;

    /*
     * Ignore cross-platform directories that can contain x86-64 binaries.
     */
    const char *platform_paths[] = {
        "/usr/i686-linux-gnu/",
        "/usr/x86_64-linux-gnu/",
        "/usr/x86_64-linux-gnux32/",
    };
    size_t num_platform_paths =
        sizeof(platform_paths) / sizeof(platform_paths[0]);
    for (size_t i = 0; i < num_platform_paths; i++) {
        size_t pp_len = strlen(platform_paths[i]);
        if (strncmp(path, platform_paths[i], pp_len) == 0)
            goto success;
    }

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror(filename);
        goto error;
    }

    elf = elf_begin(fd, ELF_C_READ, NULL);
    if (elf == NULL || elf_kind(elf) != ELF_K_ELF)
        goto success;

    GElf_Ehdr ehdr_mem;
    GElf_Ehdr *ehdr = gelf_getehdr(elf, &ehdr_mem);
    if (ehdr == NULL) {
        elf_perror(path);
        goto error;
    }

    /* Only allow x86-64 binaries */
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
        ehdr->e_machine != EM_X86_64)
        goto success;

    if (ehdr->e_type == ET_EXEC) {
        is_lib = 0;
    } else if (ehdr->e_type == ET_DYN) {
        /*
         * Unfortunately, ELF doesn't distinguish between dynamic libraries
         * and loadable modules. In order to avoid processing a huge
         * number of definitions from loadable modules, PIE executables,
         * or other binaries that aren't actual libraries, we check
         * whether the binary can be found in the hard-coded ld.so search
         * paths. This will miss some libraries in non-standard directories,
         * typically found via rpath.
         */
        const char *search_paths[] = {
            "/lib/x86_64-linux-gnu/",
            "/usr/lib/x86_64-linux-gnu/",
            "/usr/local/lib/x86_64-linux-gnu/"
        };
        size_t num_search_paths =
            sizeof(search_paths) / sizeof(search_paths[0]);
        is_lib = 0;
        for (size_t i = 0; i < num_search_paths; i++) {
            size_t sp_len = strlen(search_paths[i]);
            if (strncmp(path, search_paths[i], sp_len) == 0 &&
                strchr(path + sp_len, '/') == NULL) {
                is_lib = 1;
                break;
            }
        }
    } else {
        goto success;
    }

    /*
     * Scan ELF sections.
     */
    Elf_Data *dynamic_data = NULL;
    GElf_Shdr dynamic_shdr = { 0 };
    Elf_Data *dynsym_data = NULL;
    GElf_Shdr dynsym_shdr = { 0 };
    Elf_Data *versym_data = NULL;
    GElf_Shdr versym_shdr = { 0 };
    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr_mem;
        GElf_Shdr *shdr = gelf_getshdr(scn, &shdr_mem);
        if (shdr == NULL) {
            elf_perror(path);
            goto error;
        }
        Elf_Data *data = elf_getdata(scn, NULL);
        if (data == NULL) {
            elf_perror(path);
            goto error;
        }

        switch (shdr->sh_type) {
            case SHT_DYNAMIC:
                dynamic_data = data;
                dynamic_shdr = *shdr;
                break;
            case SHT_DYNSYM:
                dynsym_data = data;
                dynsym_shdr = *shdr;
                break;
            case SHT_GNU_versym:
                versym_data = data;
                versym_shdr = *shdr;
                break;
            default:
                break;
        }
    }

    fprintf(g_out_def,
            "DELETE FROM binary WHERE path='%s';\n",
            path);
    fprintf(g_out_def,
            "DELETE FROM definition WHERE path='%s';\n",
            path);
    fprintf(g_out_ref,
            "DELETE FROM reference WHERE path='%s';\n",
            path);
    fprintf(g_out_ref,
            "DELETE FROM so_reference WHERE path='%s';\n",
            path);

    /*
     * Extract sonames.
     */
    const char *soname = "";
    size_t num_so_ref = 0;
    if (dynamic_data) {
        size_t nentries = dynamic_shdr.sh_entsize ?
                          dynamic_shdr.sh_size / dynamic_shdr.sh_entsize :
                          0;
        for (size_t cnt = 0; cnt < nentries; ++cnt) {
            GElf_Dyn dyn_mem;
            GElf_Dyn *dyn = gelf_getdyn(dynamic_data, cnt, &dyn_mem);
            if (dyn == NULL) {
                elf_perror(path);
                goto error;
            }
            if (dyn->d_tag == DT_SONAME) {
                soname = elf_strptr(elf, dynamic_shdr.sh_link,
                                    dyn->d_un.d_val);
            } else if (dyn->d_tag == DT_NEEDED) {
                const char *needed = elf_strptr(elf, dynamic_shdr.sh_link,
                        dyn->d_un.d_val);
                fprintf(g_out_ref, "%s('%s','%s')",
                        num_so_ref ?
                            ",\n" :
                            "INSERT INTO so_reference"
                            " (path, soname) VALUES\n",
                        path, needed);
                num_so_ref += 1;
            }
        }
    }
    if (num_so_ref)
        fprintf(g_out_ref, ";\n");

    /*
     * Extract dynamic symbols.
     */
    size_t num_ref = 0;
    size_t num_def = 0;
    if (dynsym_data != NULL) {
        int nrelocs = find_copy_relocs(elf, elf_ndxscn(scn), NULL);
        if (nrelocs < 0) {
            elf_perror(path);
            goto error;
        }
        free(relocs);
        relocs = malloc(nrelocs * sizeof(relocs[0]));
        if (relocs == NULL) {
            perror(path);
            goto error;
        }
        if (find_copy_relocs(elf, elf_ndxscn(scn), relocs) < 0) {
            elf_perror(path);
            goto error;
        }

        size_t nentries = dynsym_shdr.sh_entsize ?
                          dynsym_shdr.sh_size / dynsym_shdr.sh_entsize :
                          0;
        Elf64_Half *versym_ndx = NULL;

        if (versym_data) {
            size_t vnentries = versym_shdr.sh_size / sizeof(Elf64_Half);
            if (vnentries < nentries) {
                fprintf(stderr, "%s: invalid versym section\n", path);
                goto error;
            }
            versym_ndx = versym_data ? versym_data->d_buf : NULL;
        }

        for (size_t cnt = 0; cnt < nentries; ++cnt) {
            GElf_Sym sym_mem;
            GElf_Sym *sym = gelf_getsym(dynsym_data, cnt, &sym_mem);
            if (sym == NULL) {
                elf_perror(path);
                goto error;
            }

            int type = GELF_ST_TYPE(sym->st_info);
            int type_char;
            switch (type) {
                case STT_OBJECT:
                    type_char = 'O';
                    break;
                case STT_FUNC:
                    type_char = 'F';
                    break;
                case STT_TLS:
                    type_char = 'T';
                    break;
                case STT_GNU_IFUNC:
                    type_char = 'I';
                    break;
                case STT_NOTYPE:
                case STT_SECTION:
                    continue;
                default:
                    warn(path, &warn_flag, "Unknown st_type: %d\n", type);
                    continue;
            }

            int bind = GELF_ST_BIND(sym->st_info);
            int bind_char;
            switch (bind) {
                case STB_GLOBAL:
                    bind_char = 'G';
                    break;
                case STB_WEAK:
                    bind_char = 'W';
                    break;
                case STB_GNU_UNIQUE:
                    bind_char = 'U';
                    break;
                case STB_LOCAL:
                    continue;
                default:
                    warn(path, &warn_flag, "Unknown st_bind: %d\n", bind);
                    continue;
            }

            if (versym_ndx && versym_ndx[cnt] & 0x8000)
                continue;

            const char *name = elf_strptr(elf, dynsym_shdr.sh_link,
                                          sym->st_name);
            if (name == NULL || name[0] == 0) {
                warn(path, &warn_flag, "Empty name\n");
                continue;
            }
            /*
             * Skip C++ symbols. Unfortunately, many C++ projects
             * unnecessarily export huge numbers of symbols. The mangled
             * names also tend to be long, creating a much larger database.
             */
            if (name[0] == '_' && name[1] == 'Z')
                continue;
            /* Skip golang symbols */
            if (strchr(name, '.') != NULL)
                continue;

            /* Special sections */
            if (sym->st_shndx >= SHN_LORESERVE &&
                sym->st_shndx != SHN_XINDEX)
                continue;

            int defined = 0;
            if (sym->st_shndx != SHN_UNDEF) {
                defined = 1;
                for (int i = 0; i < nrelocs; i++) {
                    if (relocs[i] == cnt) {
                        defined = 0;
                        break;
                    }
                }
            }

            if (defined && !is_lib)
                continue;

            if (defined) {
                const char *prefix = num_def ?
                         ",\n" :
                         "INSERT OR IGNORE INTO definition"
                         " (path, type, symbol, bind, size) VALUES\n";
                fprintf(g_out_def, "%s('%s','%c','%s','%c',%lu)",
                        prefix, path, type_char, name, bind_char,
                        sym->st_size);
                num_def += 1;
            } else {
                const char *prefix = num_ref ?
                         ",\n" :
                         "INSERT OR IGNORE INTO reference"
                         " (path, type, symbol, bind) VALUES\n";
                fprintf(g_out_ref, "%s('%s','%c','%s','%c')",
                        prefix, path, type_char, name, bind_char);
                num_ref += 1;
            }
        }
    }
    if (num_def)
        fprintf(g_out_def, ";\n");
    if (num_ref)
        fprintf(g_out_ref, ";\n");

    if (num_def || num_ref || num_so_ref)
        fprintf(g_out_def,
                "INSERT INTO binary (path, soname, package) VALUES\n"
                "('%s','%s','%s');\n",
                path, soname, g_package);

success:
    ret = 0;

error:
    if (relocs)
        free(relocs);
    if (elf)
        elf_end(elf);
    if (fd != -1)
        close(fd);

    return ret;
}

static void
db_command(const char *str) {
    fputs(str, g_out_def);
    fputs(str, g_out_ref);
}

int
main(int argc, const char **argv) {
    int ret = 1;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s PACKAGE_NAME DATA_DIR\n", argv[0]);
        goto error;
    }
    g_package = argv[1];
    g_data_dir = argv[2];
    g_len_data_dir = strlen(g_data_dir);

    elf_version(EV_CURRENT);

    g_out_def = fopen("update_defs.sql", "wb");
    if (g_out_def == NULL) {
        perror("update_defs.sql");
        goto error;
    }

    g_out_ref = fopen("update_refs.sql", "wb");
    if (g_out_ref == NULL) {
        perror("update_refs.sql");
        goto error;
    }

    db_command("PRAGMA synchronous = OFF;\n"
               "PRAGMA journal_mode = MEMORY;\n"
               "BEGIN TRANSACTION;\n");

    ret = nftw(g_data_dir, process_file, 32, FTW_PHYS);

    db_command("END TRANSACTION;\n");

error:
    if (g_out_ref)
        fclose(g_out_ref);
    if (g_out_def)
        fclose(g_out_def);

    return ret;
}

