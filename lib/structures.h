#include <linux/limits.h>

#ifndef _STRUCT_H
#define _STRUCT_H

#define STRINGIFY(s) STR(s)
#define STR(s) #s
#define MAXHOPS 1001
#define MAXBUFLENGTH 50
#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                     perror(source),                                 \
                     exit(EXIT_FAILURE))

typedef struct item
{
    int name;
    int dest;
} item_t;

typedef struct room
{
    int items[2];
    int *neigh;
    int n;
    int k;
} room_t;

typedef struct player
{
    int moves;
    int room;
    int picked;
} player_t;

typedef struct autosave_data
{
    char path[PATH_MAX];
    pthread_mutex_t *mx;
    room_t *map;
    item_t *items;
    player_t *p;
    int n;
} autosave_t;

typedef struct path
{
    int tab[MAXHOPS];
    unsigned int seed;
    int length;
    room_t *map;
    int n;
    int target;
    pthread_mutex_t *mx;
} path_t;

typedef struct swap
{
    pthread_mutex_t *mx;
    room_t *map;
    unsigned int seed;
    int n;
} swap_t;

#endif