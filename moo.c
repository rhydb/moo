#include "date.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>

#define TITLE_LEN 50
#define DESC_LEN 100
#define FILE_NAME_LEN 12 // year(4) + filedelim(1) + month(2) + delim(1) + day(2) + \0

static void
usage()
{
    fputs("usage: moo [year] [month] [day] [add|delete] [title] [description] [line]\n"
          "           [-d delim] [-fd delim] [-p path]\n"
          "\n"
          "  -d    title-description delimiter\n"
          "  -fd   file name delimiter\n"
          "  -p    path to read/write, will contain a moo subfolder\n", stderr);
    exit(1);
}

/* go the next item in argv that isn't an option
   this assumes all options take an argument */
static char
nextarg(int *i, int argc, char **argv)
{
    while (*i + 1 < argc) {
        *i += 1;
        if (argv[*i][0] != '-')
            return 1;

        if (*i + 1 == argc)
                usage();
        *i += 1;
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    char *store = NULL;

    char filedelim = '-';
    char eventdelim = ':';

    int i;

    char week = 0; // match files in the coming week
    char title[TITLE_LEN];
    char desc[DESC_LEN];
    struct date search;
    search.year = search.month = search.day = -1;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-')
            continue;
        if (i + 1 == argc)
            usage();
        else if (!strcmp(argv[i], "-d"))
            eventdelim = argv[++i][0];
        else if (!strcmp(argv[i], "-fd"))
            filedelim = argv[++i][0];
        else if (!strcmp(argv[i], "-p")) {
            store = argv[++i];
        }
    }

    if (!store) {
        char *data;
        if (!(data = getenv("XDG_DATA_HOME")) || data[0] == '\0') {
            const char *home;
            if (!(home = getenv("HOME"))) {
                struct passwd *pw = getpwuid(getuid());
                home = pw->pw_dir;
            }
            const size_t homelen = strlen(home);
            const char* const dir = "/.local/share/moo";
            store = malloc(homelen + strlen(dir));
            strcpy(store, home);
            strcpy(store + homelen, dir);
        } else {
            const size_t datalen = strlen(data);
            store = malloc(datalen + strlen("/moo"));
            strcpy(store, data);
            strcat(store, "/moo");
        }
    }

    size_t storelen = strlen(store);
    struct stat st = {0};
    if (stat(store, &st) == -1) {
        if (mkdir(store, 0700) == -1) {
            fprintf(stderr, "failed to create store %s: %s\n", store, strerror(errno));
            exit(1);
        }
    }

    i = 0;

    if (nextarg(&i, argc, argv)
            // when these verbs are the first argument today's date is used
            && strcmp(argv[i], "add")
            && strcmp(argv[i], "delete")
            && strcmp(argv[i], "week")) {
        if (!(search.year = atoi(argv[i]))) {
            fputs("invalid year\n", stderr);
            exit(1);
        }

        if (nextarg(&i, argc, argv)) {
            if (!(search.month = atoi(argv[i]))) {
                fputs("invalid month\n", stderr);
                exit(1);
            }

            if (nextarg(&i, argc, argv)) {
                if (!(search.day = atoi(argv[i]))) {
                    fputs("invalid day\n", stderr);
                    exit(1);
                }
            }
        }
    } else {
        if (i < argc && !strcmp(argv[i], "week")) {
            week = 1;
        } else {
            i = 0;
        }
        search.year = tm.tm_year + 1900;
        search.month = tm.tm_mon + 1;
        search.day = tm.tm_mday;
    }

    char fname[FILE_NAME_LEN];
    snprintf(fname, FILE_NAME_LEN, "%04u%c", search.year, filedelim);
    int fnamelen = 5; // year(4) + filedelim(search.1)

    if (search.month != -1) {
       snprintf(fname + fnamelen, FILE_NAME_LEN - fnamelen, "%02u%c", search.month, filedelim);
       fnamelen += 3; // month(2) + filedelim(1) 
       if (search.day != -1) {
          snprintf(fname + fnamelen, FILE_NAME_LEN - fnamelen, "%02u", search.day);
          fnamelen += 2; // day(2)
       }
    }

    char *fpath = malloc(storelen + 1 + FILE_NAME_LEN);
    if (!fpath) {
        fprintf(stderr, "failed to malloc fpath\n");
        exit(1);
    }

    strcat(fpath, store);
    strcat(fpath, "/");
    strcat(fpath, fname);

    if (nextarg(&i, argc, argv)) {
        if (!strcmp(argv[i], "add")) {
            if (nextarg(&i, argc, argv)) {
                strncpy(title, argv[i], TITLE_LEN);
                if (nextarg(&i, argc, argv)) {
                    strncpy(desc, argv[i], DESC_LEN);
                }
            }

            if (!title[0]) {
                printf("title: ");
                fflush(stdin);
                fgets(title, TITLE_LEN, stdin);
                int len = strlen(title);
                if (title[len - 1] == '\n')
                    title[len - 1] = '\0';
                if (!desc[0]) {
                    printf("description: ");
                    fflush(stdin);
                    fgets(desc, DESC_LEN, stdin);
                    int len = strlen(desc);
                    if (desc[len - 1] == '\n')
                        desc[len - 1] = '\0';
                }
            }

            FILE *file = fopen(fpath, "a");
            if (!file) {
                fprintf(stderr, "failed to open %s: %s\n", fpath, strerror(errno));
                exit(1);
            }

            fprintf(file, "%c%s%c%s\n", eventdelim, title, eventdelim, desc);

            fclose(file);
        } else if (!strcmp(argv[i++], "delete")) {
            int line;
            if (i == argc || !(line = atoi(argv[i++]))) {
                fputs("invalid line\n", stderr);
                exit(1);
            }

            FILE *file = fopen(fpath, "r");
            if (!file) {
                fprintf(stderr, "failed to open '%s': %s\n", fpath, strerror(errno));
                exit(1);
            }

            fseek(file, 0L, SEEK_END);
            size_t size = ftell(file);
            rewind(file);

            char *temp = malloc(size);
            size_t n = 0;

            char c;
            int nlcount = 1;
            while ((c = fgetc(file)) != EOF) {
                if (nlcount != line)
                    temp[n++] = c;
                if (c == '\n')
                    nlcount += 1;
            }
            temp[n] = '\0';

            fclose(file);

            if (n == 0) {
                if (remove(fpath)) {
                    fprintf(stderr, "failed to remove '%s': %s\n", fpath, strerror(errno));
                    free(temp);
                    return 1;
                }
            } else {
                file = fopen(fpath, "w");
                if (!file) {
                    fprintf(stderr, "failed to open '%s': %s\n", fpath, strerror(errno));
                    exit(1);
                }
                fputs(temp, file);
                fclose(file);
            }
            free(temp);
        }
    } else {
        DIR *storedir = opendir(store);
        if (storedir == NULL) {
            fprintf(stderr, "failed to open store '%s': %s\n", store, strerror(errno));
            exit(1);
        }

        errno = 0;
        struct dirent *ent;

        char *entpath = malloc(storelen + 1 + 256);
        if (!entpath) {
            fprintf(stderr, "failed to malloc entpath\n");
        }

        struct date nextweek = dateadd(search, 7);

        /* extract date from file name using sscanf */
        char format[256];
        sprintf(format, "%%04u%c%%02u%c%%02u", filedelim, filedelim);
        struct date fdate;

        char *title = malloc(TITLE_LEN);
        char *desc = malloc(DESC_LEN);
        while ((ent = readdir(storedir))) {
            if (ent->d_type != DT_REG)
                continue;

            if (week) {
                // extract the year, month date from the file name
                sscanf(ent->d_name, format, &fdate.year, &fdate.month, &fdate.day);
                if (!datewithin(search, fdate, nextweek))
                    continue;
            } else if (strncmp(fname, ent->d_name, fnamelen))
                continue;

            strcpy(entpath, store);
            strcpy(entpath + storelen, "/");
            strcpy(entpath + storelen + 1, ent->d_name);

            FILE *file = fopen(entpath, "r");
            if (!file) {
                fprintf(stderr, "failed to open %s: %s\n", entpath, strerror(errno));
                continue;
            }

            size_t titlelen = TITLE_LEN;
            size_t desclen = DESC_LEN;

            ssize_t titleread;
            ssize_t descread;

            size_t line = 1;

            fseek(file, 0L, SEEK_END);
            size_t size = ftell(file);
            if (size > 0) {
                printf("%s\n", ent->d_name);
                rewind(file);

                while (1) {
                    errno = 0;
                    eventdelim = fgetc(file);
                    titleread = getdelim(&title, &titlelen, eventdelim, file);
                    if (errno) {
                        fprintf(stderr, "failed to getfiledelim: %s\n", strerror(errno));
                        exit(1);
                    }
                    descread = getline(&desc, &desclen, file);
                    if (errno) {
                        fprintf(stderr, "failed to getline: %s\n", strerror(errno));
                        exit(1);
                    }

                    if (titleread == -1 || descread == -1)
                        break;
                    // remove delim
                    title[titleread - 1] = '\0';
                    if (descread > 1)
                        desc[descread - 1] = '\0';

                    printf("    %ld %s\n", line, title);
                    if (descread > 1)
                        printf("         %s\n", desc);

                    line += 1;
                }
            }

            fclose(file);
        }

        free(desc);
        free(title);
        free(entpath);

        if (errno) {
            fprintf(stderr, "failed to read store %s: %s\n", store, strerror(errno));
            exit(1);
        }

        closedir(storedir);
    }
    free(fpath);
    free(store);
}
