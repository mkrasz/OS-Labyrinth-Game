#include "structures.h"

#ifndef _GEN_H
#define _GEN_H

void generate_map(int n, char *path);
void scan(char *path, room_t **map, int *n);
void generate_dir(char *tree, char *path);
void generate_items(item_t *items, room_t *rooms, int count, int n);

#endif