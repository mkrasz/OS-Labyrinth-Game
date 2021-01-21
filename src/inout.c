#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "../lib/structures.h"
#include "../lib/game.h"
#include "../lib/generating.h"
#include "../lib/inout.h"

void usage(char *name)
{
    fprintf(stderr, "USAGE:%s [-b backup-path] \n", name);
}

ssize_t bulk_read(int fd, void *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len; //EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, void *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void write_int(int fd, int num)
{
    int *pnum = &num;
    if (bulk_write(fd, pnum, sizeof(int)) < 0)
        ERR("write");
}

void write_player(int fd, player_t p)
{
    player_t *pp = &p;
    if (bulk_write(fd, pp, sizeof(player_t)) < 0)
        ERR("write");
}

int read_int(int fd)
{
    int *buf = (int *)malloc(sizeof(int));
    if (buf == NULL)
        ERR("malloc");
    if (bulk_read(fd, buf, sizeof(int)) < 0)
        ERR("read");
    int res = *buf;
    free(buf);
    return res;
}

void print_item(item_t it)
{
    printf(" item nr %d -> %d", it.name, it.dest);
}

void print_status(player_t player, room_t *rooms, item_t *items)
{
    printf("----------------------------------------\n");
    int room_nr = player.room;
    printf(" Current room: %d\n", room_nr);
    printf(" Current item: ");
    if (player.picked >= 0)
        print_item(items[player.picked]);
    else
        printf("none");
    printf("\n Available rooms:");
    for (int i = 0; i < rooms[room_nr].n; i++)
        printf(" %d", rooms[room_nr].neigh[i]);
    printf("\n Available items:");
    switch (rooms[room_nr].k)
    {
    case 0:
        printf(" none");
        break;
    case 1:
        print_item(items[rooms[room_nr].items[0]]);
        break;
    case 2:
        print_item(items[rooms[room_nr].items[0]]);
        printf(", ");
        print_item(items[rooms[room_nr].items[1]]);
    }
    printf("\n----------------------------------------\n");
}

void load_map(int fd, room_t **map, int *n)
{
    *n = read_int(fd);
    *map = (room_t *)malloc(sizeof(room_t) * *n);
    if (map == NULL)
        ERR("malloc");
    for (int i = 0; i < *n; i++)
    {
        int nr = read_int(fd);
        (*map)[nr].n = read_int(fd);
        if (((*map)[nr].neigh = (int *)malloc(sizeof(int) * (*map)[nr].n)) == NULL)
            ERR("malloc");
        for (int i = 0; i < (*map)[nr].n; i++)
            (*map)[nr].neigh[i] = read_int(fd);
    }
}

void save_map(int fd, room_t *map, int n)
{
    write_int(fd, n);
    for (int i = 0; i < n; i++)
    {
        write_int(fd, i);
        write_int(fd, map[i].n);
        for (int j = 0; j < map[i].n; j++)
            write_int(fd, map[i].neigh[j]);
    }
}

void save_status(char *path, room_t *map, int n, item_t *items, player_t p)
{
    int out;
    if ((out = TEMP_FAILURE_RETRY(open(path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open");
    save_map(out, map, n);
    for (int i = 0; i < n; i++)
    {
        write_int(out, map[i].items[0]);
        write_int(out, map[i].items[1]);
    }
    for (int i = 0; i < 3 * n / 2; i++)
    {
        write_int(out, i);
        write_int(out, items[i].dest);
    }
    write_player(out, p);
    if (TEMP_FAILURE_RETRY(close(out)))
        ERR("close");
    printf(" Status saved to: %s\n", path);
}