#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <signal.h>

#include "../lib/structures.h"
#include "../lib/game.h"
#include "../lib/generating.h"
#include "../lib/inout.h"

int main(int argc, char **argv)
{
    char buf[MAXBUFLENGTH], path[PATH_MAX], backup[PATH_MAX], tree[PATH_MAX], c;
    bool isSet = false;
    srand(time(NULL));
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    while ((c = getopt(argc, argv, "b:")) != -1)
        switch (c)
        {
        case 'b':
            strcpy(backup, optarg);
            isSet = true;
            break;
        case '?':
        default:
            usage(argv[0]);
        }
    if (!isSet)
    {
        if (getenv("GAME_AUTOSAVE"))
            strcpy(backup, getenv("GAME_AUTOSAVE"));
        else
        {
            strcpy(backup, getenv("HOME"));
            strcat(backup, "/.game-autosave");
        }
    }
    printf("\nMain menu options:\n map-from-dir-tree path-d path f\n generate-random-map n path\n start-game path\n load-game path\n exit\n");
    do
    {
        printf("Main menu: ");
        scanf("%" STRINGIFY(MAXBUFLENGTH) "s", buf);
        if (strcmp(buf, "start-game") == 0)
        {
            scanf("%" STRINGIFY(PATH_MAX) "s", path);
            start_game(path, backup);
        }
        else if (strcmp(buf, "load-game") == 0)
        {
            scanf("%" STRINGIFY(PATH_MAX) "s", path);
            load_game(path, backup);
        }
        else if (strcmp(buf, "generate-random-map") == 0)
        {
            int n;
            scanf("%d", &n);
            if (n <= 0)
                printf("Wrong n parameter\n");
            scanf("%" STRINGIFY(PATH_MAX) "s", path);
            generate_map(n, path);
        }
        else if (strcmp(buf, "map-from-dir-tree") == 0)
        {
            scanf("%" STRINGIFY(PATH_MAX) "s", tree);
            scanf("%" STRINGIFY(PATH_MAX) "s", path);
            generate_dir(tree, path);
        }
    } while (strcmp(buf, "exit") != 0);
    return EXIT_SUCCESS;
}