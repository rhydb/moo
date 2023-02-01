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
          "           [-dmy day/month/year] [-td delim] [-fd delim] [-p path] [-i days] [-o days]\n"
          "\n"
          "  -td    title-description delimiter\n"
          "  -fd    file name delimiter\n"
          "  -p     path to read/write, will contain a moo subfolder\n"
          "  -i     number of days to include, can be negative\n"
          "  -o     offset today's date by a number of days, can be negative\n", stderr);
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

static int
eatoi(const char *str, const char *error)
{
    int res = atoi(str);
    if (!res) {
        fputs(error, stderr);
        exit(1);
    }
    return res;
}

int
main(int argc, char *argv[])
{
    char *storepath = NULL;

    char filedelim = '-';
    char eventdelim = ':';

    int range = 0; // match files within a certain number of days
    int offset = 0;
    struct date search;
    search.year = search.month = search.day = 0;

    int i;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-')
            continue;
        if (i + 1 == argc)
            usage();
        else if (!strcmp(argv[i], "-y"))
            search.year = eatoi(argv[++i], "invalid year\n");
        else if (!strcmp(argv[i], "-m"))
            search.month = eatoi(argv[++i], "invalid month\n");
        else if (!strcmp(argv[i], "-d"))
            search.day = eatoi(argv[++i], "invalid day\n");
        else if (!strcmp(argv[i], "-td"))
            eventdelim = argv[++i][0];
        else if (!strcmp(argv[i], "-fd"))
            filedelim = argv[++i][0];
        else if (!strcmp(argv[i], "-p"))
            storepath = argv[++i];
        else if (!strcmp(argv[i], "-i"))
            range = eatoi(argv[++i], "invalid range\n");
        else if (!strcmp(argv[i], "-o"))
            offset = eatoi(argv[++i], "invalid offset\n");
    }

    // get the store directory following XDG if one wasn't provided
    if (!storepath) {
        char *data;
        if (!(data = getenv("XDG_DATA_HOME")) || data[0] == '\0') {
            // use $HOME/.local/share/moo instead
            const char *home;
            if (!(home = getenv("HOME"))) {
                struct passwd *pw = getpwuid(getuid());
                home = pw->pw_dir;
            }
            const size_t homelen = strlen(home);
            const char* const dir = "/.local/share/moo";
            storepath = malloc(homelen + strlen(dir));
            strcpy(storepath, home);
            strcpy(storepath + homelen, dir);
        } else {
            const size_t datalen = strlen(data);
            storepath = malloc(datalen + strlen("/moo"));
            strcpy(storepath, data);
            strcat(storepath, "/moo");
        }
    }

    // create the store directory if it doesn't exist
    {
        struct stat st = {0};
        if (stat(storepath, &st) == -1) {
            if (mkdir(storepath, 0700) == -1) {
                fprintf(stderr, "failed to create store %s: %s\n", storepath, strerror(errno));
                exit(1);
            }
        }
    }

    i = 0;

    if (nextarg(&i, argc, argv)
            // when these verbs are the first argument today's date is used
            && strcmp(argv[i], "add")
            && strcmp(argv[i], "delete")
            && strcmp(argv[i], "week")) {

        search.year = eatoi(argv[i], "invalid year\n");
        if (nextarg(&i, argc, argv)) {
            search.month = eatoi(argv[i], "invalid month\n");
            if (nextarg(&i, argc, argv))
                search.day = eatoi(argv[i], "invalid day\n");
        }
    } else {
        if (i < argc && !strcmp(argv[i], "week"))
            range = 7;
        else
            i = 0;

#define COALESCE(x, y) (x ? x : y) // return x if truthy else y
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        struct date date = {
            .year = COALESCE(search.year, tm.tm_year + 1900),
            .month = COALESCE(search.month, tm.tm_mon + 1),
            .day = COALESCE(search.day, tm.tm_mday),
        };
        search = dateadd(date, offset);
    }

    // build the file name string
    char fname[FILE_NAME_LEN];
    snprintf(fname, FILE_NAME_LEN, "%04u%c", search.year, filedelim);
    int fnamelen = 5; // year(4) + filedelim(search.1)

    if (search.month) {
       snprintf(fname + fnamelen, FILE_NAME_LEN - fnamelen, "%02u%c", search.month, filedelim);
       fnamelen += 3; // month(2) + filedelim(1) 
       if (search.day) {
          snprintf(fname + fnamelen, FILE_NAME_LEN - fnamelen, "%02u", search.day);
          fnamelen += 2; // day(2)
       }
    }

    size_t storepathlen = strlen(storepath);
    char *fpath = malloc(storepathlen + 1 + FILE_NAME_LEN);
    if (!fpath) {
        fprintf(stderr, "failed to malloc fpath\n");
        exit(1);
    }

    strcpy(fpath, storepath);
    strcat(fpath, "/");
    strcat(fpath, fname);

    if (nextarg(&i, argc, argv)) {
        char title[TITLE_LEN];
        char desc[DESC_LEN];

        if (!strcmp(argv[i], "add")) {
            // get the title and description from the next arguments
            if (nextarg(&i, argc, argv)) {
                strncpy(title, argv[i], TITLE_LEN);
                if (nextarg(&i, argc, argv)) {
                    strncpy(desc, argv[i], DESC_LEN);
                }
            }

            // if there weren't any arguments prompt the user
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
                    len = strlen(desc);
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
            if (i == argc) {
                fputs("delete requires a number or 'all'\n", stderr);
                exit(1);
            }

            if (!strcmp(argv[i], "all")) {
                line = -1;
            } else if (!(line = atoi(argv[i]))) {
                fputs("invalid line\n", stderr);
                exit(1);
            }
            i += 1;

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
                if (line != -1 && line != nlcount)
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
        DIR *storedir = opendir(storepath);
        if (storedir == NULL) {
            fprintf(stderr, "failed to open store '%s': %s\n", storepath, strerror(errno));
            exit(1);
        }

        errno = 0;
        struct dirent *ent;

        char *entrypath = malloc(storepathlen + 1 + 256);
        if (!entrypath) {
            fprintf(stderr, "failed to malloc entpath\n");
        }

        struct date nextdate = dateadd(search, range);
        if (datelt(nextdate, search)) {
            // if nextdate is before search, swap them
            struct date temp = nextdate;
            nextdate = search;
            search = temp;
        }

        /* extract date from file name using sscanf */
        char format[256];
        sprintf(format, "%%04u%c%%02u%c%%02u", filedelim, filedelim);
        struct date fdate;

        char *title = malloc(TITLE_LEN);
        char *desc = malloc(DESC_LEN);
        while ((ent = readdir(storedir))) {
            if (ent->d_type != DT_REG)
                continue;

            if (range) {
                // extract the year, month date from the file name
                sscanf(ent->d_name, format, &fdate.year, &fdate.month, &fdate.day);
                if (!datewithin(search, fdate, nextdate))
                    continue;
            } else if (strncmp(fname, ent->d_name, fnamelen))
                continue;

            strcpy(entrypath, storepath);
            strcpy(entrypath + storepathlen, "/");
            strcpy(entrypath + storepathlen + 1, ent->d_name);

            FILE *file = fopen(entrypath, "r");
            if (!file) {
                fprintf(stderr, "failed to open %s: %s\n", entrypath, strerror(errno));
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
        free(entrypath);

        if (errno) {
            fprintf(stderr, "failed to read store %s: %s\n", storepath, strerror(errno));
            exit(1);
        }

        closedir(storedir);
    }
    free(fpath);
    free(storepath);
}
