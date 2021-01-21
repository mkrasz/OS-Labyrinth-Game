#include "structures.h"

#ifndef _INOUT_H
#define _INOUT_H

void usage(char *name);
ssize_t bulk_read(int fd, void *buf, size_t count);
ssize_t bulk_write(int fd, void *buf, size_t count);
void write_int(int fd, int num);
void write_player(int fd, player_t p);
int read_int(int fd);
void print_item(item_t it);
void print_status(player_t player, room_t *rooms, item_t *items);
void load_map(int fd, room_t **map, int *n);
void save_map(int fd, room_t *map, int n);
void save_status(char *path, room_t *map, int n, item_t *items, player_t p);

#endif