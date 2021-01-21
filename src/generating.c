#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <linux/limits.h>

#include "../lib/structures.h"
#include "../lib/game.h"
#include "../lib/generating.h"
#include "../lib/inout.h"

#define MAXFD 20

void generate_map(int n, char *path)
{
    room_t *map = (room_t *)malloc(sizeof(room_t) * n);
    if (n == 1)
    {
        map->n = 0;
        map->neigh = NULL;
    }
    else
    {
        map[0].n = map[n - 1].n = 1;
        if ((map[0].neigh = (int *)malloc(sizeof(int))) == NULL)
            ERR("malloc");
        map[0].neigh[0] = 1;
        if ((map[n - 1].neigh = (int *)malloc(sizeof(int))) == NULL)
            ERR("malloc");
        map[n - 1].neigh[0] = n - 2;
        for (int i = 1; i < n - 1; i++)
        {
            map[i].n = 2;
            if ((map[i].neigh = (int *)malloc(sizeof(int) * 2)) == NULL)
                ERR("malloc");
            map[i].neigh[0] = i - 1;
            map[i].neigh[1] = i + 1;
        }
        if (n != 2)
        {
            for (int i = 0; i < n; i++)
            {
                for (int j = i + 2; j < n; j++)
                {
                    if (rand() % 50 == 0)
                    {
                        map[i].n++;
                        if ((map[i].neigh = (int *)realloc(map[i].neigh, map[i].n * sizeof(room_t))) == NULL)
                            ERR("malloc");
                        map[i].neigh[map[i].n - 1] = j;
                        map[j].n++;
                        if ((map[j].neigh = (int *)realloc(map[j].neigh, map[j].n * sizeof(room_t))) == NULL)
                            ERR("malloc");
                        map[j].neigh[map[j].n - 1] = i;
                    }
                }
            }
        }
    }
    int out;
    if ((out = TEMP_FAILURE_RETRY(open(path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open");
    save_map(out, map, n);
    if (TEMP_FAILURE_RETRY(close(out)))
        ERR("close");
    for (int i = 0; i < n; i++)
        free(map[i].neigh);
    free(map);
}

void scan(char *path, room_t **map, int *n)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    int cur = *n - 1, rooms_count = 0;
    char **npath = NULL;
    char pathtonewfile[PATH_MAX];
    if ((dirp = opendir(path)) == NULL)
        ERR("opendir");
    do
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
        {
            strcpy(pathtonewfile, path);
            strcat(pathtonewfile, "/");
            strcat(pathtonewfile, dp->d_name);
            if (lstat(pathtonewfile, &filestat))
                ERR("lstat");
            if (S_ISDIR(filestat.st_mode))
            {
                if (strcmp(dp->d_name, "..") != 0 && strcmp(dp->d_name, ".") != 0)
                {
                    ++rooms_count;
                    if (npath == NULL)
                        npath = (char **)malloc(sizeof(char *) * rooms_count);
                    else
                        npath = (char **)realloc(npath, sizeof(char *) * rooms_count);
                    npath[rooms_count - 1] = (char *)malloc(sizeof(char) * PATH_MAX);
                    strcpy(npath[rooms_count - 1], pathtonewfile);
                }
            }
        }
    } while (dp != NULL);
    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");
    for (int i = 0; i < rooms_count; i++)
    {
        ++(*n);
        if ((*map = (room_t *)realloc(*map, sizeof(room_t) * *n)) == NULL)
            ERR("realloc");
        (*map)[cur].n++;
        if (((*map)[cur].neigh = (int *)realloc((*map)[cur].neigh, sizeof(int) * (*map)[cur].n)) == NULL)
            ERR("realloc");
        (*map)[cur].neigh[(*map)[cur].n - 1] = *n - 1;
        (*map)[*n - 1].n = 1;
        if (((*map)[*n - 1].neigh = (int *)malloc(sizeof(int))) == NULL)
            ERR("malloc");
        (*map)[*n - 1].neigh[0] = cur;
        scan(npath[i], map, n);
    }
    for (int i = 0; i < rooms_count; i++)
        free(npath[i]);
    free(npath);
}

void generate_dir(char *tree, char *path)
{
    room_t *map = (room_t *)malloc(sizeof(room_t));
    if (map == NULL)
        ERR("malloc");
    map[0].n = 0;
    map[0].neigh = NULL;
    int n = 1;
    scan(tree, &map, &n);
    int out;
    if ((out = TEMP_FAILURE_RETRY(open(path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open");
    save_map(out, map, n);
    if (TEMP_FAILURE_RETRY(close(out)))
        ERR("close");
    for (int i = 0; i < n; i++)
        free(map[i].neigh);
    free(map);
}

void generate_items(item_t *items, room_t *rooms, int count, int n)
{
    int *dest = (int *)malloc(sizeof(int) * n);
    if (dest == NULL)
        ERR("malloc");
    for (int i = 0; i < n; i++)
    {
        dest[i] = 0;
        rooms[i].k = 0;
    }
    int random;
    for (int i = 0; i < count; i++)
    {
        items[i].name = i;
        do
        {
            random = rand() % n;
        } while (dest[random] >= 2);
        items[i].dest = random;
        dest[random]++;
        do
        {
            random = rand() % n;
        } while (rooms[random].k >= 2);
        rooms[random].items[rooms[random].k++] = i;
    }
    free(dest);
}