#include <stdbool.h>
#include "structures.h"

#ifndef _GAME_H
#define _GAME_H

#define ELAPSED(start, end) ((end).tv_sec - (start).tv_sec) + (((end).tv_nsec - (start).tv_nsec) * 1.0e-9)

int move(player_t *p, room_t r, int nr);
int pick_up(player_t *p, room_t *r, int nr);
int drop(player_t *p, room_t *r, int nr);
void *swap_items(void *voidArg);
void cleaner(void *args);
void *autosave_thread(void *voidArg);
void *path_thread(void *voidArg);
void find_path(int start, int target, int threads, room_t *rooms, int count, pthread_mutex_t *roomsmx, unsigned int seed);
void game_menu(player_t *p, room_t *rooms, item_t *items, pthread_mutex_t *workmx, int rooms_count, pthread_t autosave, pthread_mutex_t *roomsmx, unsigned int seed);
void game(player_t *p, room_t *rooms, item_t *items, int rooms_count, char *backup);
void load_game(char *path, char *backup);
void start_game(char *path, char *backup);
bool is_won(item_t *items, room_t *rooms, int rooms_count);

#endif