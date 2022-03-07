#define _XOPEN_SOURCE 500

#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libelf.h>
#include <gelf.h>

#ifndef R_X86_64_COPY
#define R_X86_64_COPY 5
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
    size_t num_def;
    size_t num_ref;

    if (typeflag != FTW_F)
        return 0;

    if (strncmp(filename, g_data_dir, g_len_data_dir) != 0 ||
        filename[g_len_data_dir] != '/') {
        fprintf(stderr, "%s: doesn't start with '%s/'", filename, g_data_dir);
        goto error;
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
        elf_perror(filename);
        goto error;
    }

    if (ehdr->e_type == ET_EXEC) {
        is_lib = 0;
    } else if (ehdr->e_type == ET_DYN) {
        /*
         * An INTERP program header identifies PIE executables.
         */
        is_lib = 1;
        for (size_t cnt = 0; cnt < ehdr->e_phnum; cnt++) {
            GElf_Phdr phdr_mem;
            GElf_Phdr *phdr = gelf_getphdr(elf, cnt, &phdr_mem);
            if (phdr == NULL) {
                elf_perror(filename);
                goto error;
            }
            if (phdr->p_type == PT_INTERP) {
                is_lib = 0;
                break;
            }
        }
    } else {
        goto success;
    }

    fprintf(g_out_def,
            "DELETE FROM definition WHERE file='%s';\n",
            filename + g_len_data_dir);
    fprintf(g_out_ref,
            "DELETE FROM reference WHERE file='%s';\n",
            filename + g_len_data_dir);
    fprintf(g_out_ref,
            "DELETE FROM reference WHERE file='%s';\n",
            filename + g_len_data_dir);

    const char *soname = NULL;
    Elf_Scn *scn = NULL;
    num_ref = 0;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr_mem;
        GElf_Shdr *shdr = gelf_getshdr(scn, &shdr_mem);
        if (shdr == NULL) {
            elf_perror(filename);
            goto error;
        }

        if (shdr->sh_type != SHT_DYNAMIC)
            continue;

        size_t nentries = shdr->sh_entsize ?
                          shdr->sh_size / shdr->sh_entsize :
                          0;
        Elf_Data *data = elf_getdata(scn, NULL);
        if (data == NULL) {
            elf_perror(filename);
            goto error;
        }

        for (size_t cnt = 0; cnt < nentries; ++cnt) {
            GElf_Dyn dyn_mem;
            GElf_Dyn *dyn = gelf_getdyn(data, cnt, &dyn_mem);
            if (dyn == NULL) {
                elf_perror(filename);
                goto error;
            }
            if (dyn->d_tag == DT_SONAME) {
                if (is_lib)
                    soname = elf_strptr(elf, shdr->sh_link, dyn->d_un.d_val);
            } else if (dyn->d_tag == DT_NEEDED) {
                const char *needed = elf_strptr(elf, shdr->sh_link,
                        dyn->d_un.d_val);
                fprintf(g_out_ref, "%s('%s','%s')",
                        num_ref ?
                            ",\n" :
                            "INSERT INTO so_reference"
                            " (file, soname) VALUES\n",
                        filename + g_len_data_dir,
                        needed);
                num_ref += 1;
            }
        }

        break;
    }

    if (num_ref)
        fprintf(g_out_ref, ";\n");

    fprintf(g_out_ref,
            "INSERT INTO file (file, soname, package) VALUES\n"
            "('%s','%s','%s');\n",
            filename + g_len_data_dir,
            soname ? soname : "",
            g_package);

    scn = NULL;
    num_ref = 0;
    num_def = 0;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr_mem;
        GElf_Shdr *shdr = gelf_getshdr(scn, &shdr_mem);
        if (shdr == NULL) {
            elf_perror(filename);
            goto error;
        }

        if (shdr->sh_type != SHT_DYNSYM)
            continue;

        int nrelocs = find_copy_relocs(elf, elf_ndxscn(scn), NULL);
        if (nrelocs < 0) {
            elf_perror(filename);
            goto error;
        }
        free(relocs);
        relocs = malloc(nrelocs * sizeof(relocs[0]));
        if (relocs == NULL) {
            perror(filename);
            goto error;
        }
        if (find_copy_relocs(elf, elf_ndxscn(scn), relocs) < 0) {
            elf_perror(filename);
            goto error;
        }

        size_t nentries = shdr->sh_entsize ?
                          shdr->sh_size / shdr->sh_entsize :
                          0;
        Elf_Data *data = elf_getdata(scn, NULL);
        if (data == NULL) {
            elf_perror(filename);
            goto error;
        }

        for (size_t cnt = 0; cnt < nentries; ++cnt) {
            GElf_Sym sym_mem;
            GElf_Sym *sym = gelf_getsym(data, cnt, &sym_mem);
            if (data == NULL) {
                elf_perror(filename);
                goto error;
            }
            int type = GELF_ST_TYPE(sym->st_info);
            int bind = GELF_ST_BIND(sym->st_info);

            if ((type != STT_OBJECT && type != STT_FUNC) ||
                (bind != STB_GLOBAL))
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

            FILE *out;
            const char *prefix;
            if (defined) {
                out = g_out_def;
                prefix = num_def ?
                         ",\n" :
                         "INSERT INTO definition"
                         " (symbol, type, file) VALUES\n";
                num_def += 1;
            } else {
                out = g_out_ref;
                prefix = num_ref ?
                         ",\n" :
                         "INSERT INTO reference"
                         " (symbol, type, file) VALUES\n";
                num_ref += 1;
            }
            fprintf(out, "%s('%s','%c','%s')",
                    prefix,
                    elf_strptr(elf, shdr->sh_link, sym->st_name),
                    type == STT_FUNC ? 'F' : 'O',
                    filename + g_len_data_dir);
        }
    }

    if (num_def)
        fprintf(g_out_def, ";\n");
    if (num_ref)
        fprintf(g_out_ref, ";\n");

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

