#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>

#include "../lib/structures.h"
#include "../lib/game.h"
#include "../lib/generating.h"
#include "../lib/inout.h"

int move(player_t *p, room_t r, int nr)
{
    for (int i = 0; i < r.n; i++)
    {
        if (r.neigh[i] == nr)
        {
            p->room = nr;
            return 1;
        }
    }
    return 0;
}

int pick_up(player_t *p, room_t *r, int nr)
{
    if (r->k == 0)
        return 0;
    if (r->items[0] == nr)
    {
        p->picked = nr;
        r->items[0] = r->items[1];
        r->items[1] = -1;
        r->k--;
    }
    else if (r->items[1] == nr)
    {
        p->picked = nr;
        r->items[1] = -1;
        r->k--;
    }
    else
        return 0;
    return 1;
}

int drop(player_t *p, room_t *r, int nr)
{
    if (p->picked != nr || r->k == 2)
        return 0;
    p->picked = -1;
    r->items[r->k++] = nr;
    return 1;
}

void *swap_items(void *voidArg)
{
    swap_t *swapArg = (swap_t *)voidArg;
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    int i, j, m, n, temp, nr;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    while (1)
    {
        if (sigwait(&mask, &nr))
            ERR("sigwait");
        if (nr == SIGUSR1)
        {
            i = rand_r(&swapArg->seed) % swapArg->n;
            do
            {
                j = rand_r(&swapArg->seed) % swapArg->n;
            } while (j == i);
            pthread_mutex_lock(swapArg->mx);
            while (swapArg->map[i].k == 0)
                i = rand_r(&swapArg->seed) % swapArg->n;
            while (swapArg->map[j].k == 0 || i == j)
                j = rand_r(&swapArg->seed) % swapArg->n;
            m = rand_r(&swapArg->seed) % swapArg->map[i].k;
            n = rand_r(&swapArg->seed) % swapArg->map[j].k;
            temp = swapArg->map[i].items[m];
            swapArg->map[i].items[n] = swapArg->map[j].items[n];
            swapArg->map[j].items[n] = temp;
            pthread_mutex_unlock(swapArg->mx);
            printf("\n Swapped item %d in room %d with item %d in room %d\n", m, i, n, j);
        }
    }
    return NULL;
}

void cleaner(void *args)
{
    pthread_mutex_t *mx = (pthread_mutex_t *)args;
    pthread_mutex_unlock(mx);
}

void *autosave_thread(void *voidArg)
{
    autosave_t *saveArg = (autosave_t *)voidArg;
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    int nr;
    struct timespec last_save, t;
    if (clock_gettime(CLOCK_REALTIME, &last_save))
        ERR("clock_gettime");
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGUSR2);
    while (1)
    {
        alarm(60);
        if (sigwait(&mask, &nr))
            ERR("sigwait");
        if (clock_gettime(CLOCK_REALTIME, &t))
            ERR("clock_gettime");
        if (nr == SIGALRM && ELAPSED(last_save, t) >= 60)
        {
            pthread_cleanup_push(cleaner, saveArg->mx);
            pthread_mutex_lock(saveArg->mx);
            printf("\n Autosaving...\n");
            save_status(saveArg->path, saveArg->map, saveArg->n, saveArg->items, *(saveArg->p));
            pthread_cleanup_pop(1);
            printf(" Done!\n Back to game menu: ");
            fflush(stdout);
        }
    }
    return NULL;
}

void *path_thread(void *voidArg)
{
    path_t *pathArg = (path_t *)voidArg;
    int cur = pathArg->tab[0], next, prev;
    while (pathArg->length <= MAXHOPS)
    {
        prev = cur;
        pthread_mutex_lock(&(pathArg->mx[cur]));
        next = rand_r(&pathArg->seed) % pathArg->map[cur].n;
        cur = pathArg->map[cur].neigh[next];
        pthread_mutex_unlock(&(pathArg->mx[prev]));
        pathArg->tab[++pathArg->length] = cur;
        if (cur == pathArg->target)
            break;
    }
    return NULL;
}

void find_path(int start, int target, int threads, room_t *rooms, int count, pthread_mutex_t *roomsmx, unsigned int seed)
{
    pthread_t *tids = (pthread_t *)malloc(sizeof(pthread_t) * threads);
    if (tids == NULL)
        ERR("malloc");
    path_t *paths = (path_t *)malloc(sizeof(path_t) * threads);
    if (paths == NULL)
        ERR("malloc");
    for (int i = 0; i < threads; i++)
    {
        paths[i].length = 0;
        paths[i].tab[0] = start;
        paths[i].seed = rand_r(&seed);
        paths[i].map = rooms;
        paths[i].n = count;
        paths[i].target = target;
        paths[i].mx = roomsmx;
        if (pthread_create(&tids[i], NULL, path_thread, &paths[i]) != 0)
            ERR("pthread_create");
    }
    int min = MAXHOPS, minindex;
    for (int i = 0; i < threads; i++)
    {
        if (pthread_join(tids[i], NULL) != 0)
            ERR("join");
        if (paths[i].length < min)
        {
            min = paths[i].length;
            minindex = i;
        }
    }
    if (min < MAXHOPS)
    {
        printf(" Shortest found path (length %d):\n", paths[minindex].length);
        for (int i = 0; i < min; i++)
            printf(" %d ->", paths[minindex].tab[i]);
        printf(" %d\n", target);
    }
    else
        printf(" None of the %d threads found %d room\n", threads, target);
    free(paths);
    free(tids);
}

void game_menu(player_t *p, room_t *rooms, item_t *items, pthread_mutex_t *workmx, int rooms_count, pthread_t autosave, pthread_mutex_t *roomsmx, unsigned int seed)
{
    char buf[MAXBUFLENGTH], path[PATH_MAX];
    int nr, target;
    printf("\nGame menu options:\n move-to x\n pick-up y\n drop z\n save path\n find-path k x\n quit\n");
    do
    {
        print_status(*p, rooms, items);
        printf("Game menu: ");
        scanf("%" STRINGIFY(MAXBUFLENGTH) "s", buf);
        if (strcmp(buf, "move-to") == 0)
        {
            scanf("%d", &nr);
            pthread_mutex_lock(workmx);
            if (move(p, rooms[p->room], nr) == 0)
                printf(" Unable to move there\n");
            else
                p->moves++;
            pthread_mutex_unlock(workmx);
        }
        else if (strcmp(buf, "pick-up") == 0)
        {
            scanf("%d", &nr);
            pthread_mutex_lock(workmx);
            if (pick_up(p, &rooms[p->room], nr) == 0)
                printf(" Unable to pick-up\n");
            pthread_mutex_unlock(workmx);
        }
        else if (strcmp(buf, "drop") == 0)
        {
            scanf("%d", &nr);
            pthread_mutex_lock(workmx);
            if (drop(p, &rooms[p->room], nr) == 0)
                printf(" Unable to drop this item here\n");
            if (is_won(items, rooms, rooms_count))
            {
                printf(" You've won the game with %d moves!\n", p->moves);
                break;
            }
            pthread_mutex_unlock(workmx);
        }
        else if (strcmp(buf, "save") == 0)
        {
            scanf("%" STRINGIFY(PATH_MAX) "s", path);
            pthread_mutex_lock(workmx);
            save_status(path, rooms, rooms_count, items, *p);
            if (pthread_kill(autosave, SIGUSR2) < 0)
                ERR("pthread_kill");
            pthread_mutex_unlock(workmx);
        }
        else if (strcmp(buf, "find-path") == 0)
        {
            scanf("%d %d", &nr, &target);
            pthread_mutex_lock(workmx);
            if (target >= rooms_count || target < 0)
                printf(" Unable to find a path to %d\n", target);
            else if (target == p->room)
                printf(" You're already here!\n");
            else
                find_path(p->room, target, nr, rooms, rooms_count, roomsmx, seed);
            pthread_mutex_unlock(workmx);
        }
    } while (strcmp(buf, "quit") != 0);
}

void game(player_t *p, room_t *rooms, item_t *items, int rooms_count, char *backup)
{
    if (is_won(items, rooms, rooms_count))
    {
        printf(" The game was already won!\n");
        return;
    }
    unsigned int seed = rand();
    pthread_mutex_t workmx = PTHREAD_MUTEX_INITIALIZER;
    pthread_t swapping, autosave;
    swap_t swap_arg = {
        .map = rooms,
        .mx = &workmx,
        .seed = rand(),
        .n = rooms_count};
    autosave_t save_arg = {
        .mx = &workmx,
        .p = p,
        .map = rooms,
        .items = items,
        .n = rooms_count};
    strcpy(save_arg.path, backup);
    if (pthread_create(&swapping, NULL, swap_items, &swap_arg) != 0)
        ERR("pthread_create");
    if (pthread_create(&autosave, NULL, autosave_thread, &save_arg) != 0)
        ERR("pthread_create");
    pthread_mutex_t *roomsmx = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * rooms_count);
    if (roomsmx == NULL)
        ERR("malloc");
    for (int i = 0; i < rooms_count; i++)
        if (pthread_mutex_init(&roomsmx[i], NULL))
            ERR("mutex_init");
    game_menu(p, rooms, items, &workmx, rooms_count, autosave, roomsmx, seed);
    pthread_cancel(swapping);
    pthread_mutex_lock(&workmx);
    pthread_cancel(autosave);
    pthread_mutex_unlock(&workmx);
    for (int i = 0; i < rooms_count; i++)
        pthread_mutex_destroy(&roomsmx[i]);
    free(roomsmx);
}

void load_game(char *path, char *backup)
{
    int in;
    if ((in = TEMP_FAILURE_RETRY(open(path, O_RDONLY))) < 0)
        ERR("open");
    int rooms_count, items_count, roomsIn = 0, itemsIn = 0, nr;
    room_t *map = NULL;
    player_t player;
    load_map(in, &map, &rooms_count);
    items_count = 3 * rooms_count / 2;
    item_t *items = (item_t *)malloc(sizeof(item_t) * items_count);
    if (items == NULL)
        ERR("malloc");
    while (roomsIn < rooms_count)
    {
        map[roomsIn].items[0] = read_int(in);
        map[roomsIn].items[1] = read_int(in);
        if (map[roomsIn].items[1] == -1)
            if (map[roomsIn].items[0] == -1)
                map[roomsIn].k = 0;
            else
                map[roomsIn].k = 1;
        else
            map[roomsIn].k = 2;
        roomsIn++;
    }
    while (itemsIn < items_count)
    {
        nr = read_int(in);
        items[nr].name = nr;
        items[nr].dest = read_int(in);
        itemsIn++;
    }
    if (bulk_read(in, &player, sizeof(player_t)) < 0)
        ERR("read");
    if (TEMP_FAILURE_RETRY(close(in)))
        ERR("close");
    game(&player, map, items, rooms_count, backup);
    for (int i = 0; i < rooms_count; i++)
        free(map[i].neigh);
    free(map);
    free(items);
}

void start_game(char *path, char *backup)
{
    int in;
    if ((in = TEMP_FAILURE_RETRY(open(path, O_RDONLY))) < 0)
        ERR("open");
    int rooms_count, items_count;
    room_t *map = NULL;
    load_map(in, &map, &rooms_count);
    items_count = 3 * rooms_count / 2;
    item_t *items = (item_t *)malloc(sizeof(item_t) * items_count);
    if (items == NULL)
        ERR("malloc");
    if (TEMP_FAILURE_RETRY(close(in)))
        ERR("close");
    generate_items(items, map, items_count, rooms_count);
    player_t p = {
        .moves = 0,
        .picked = -1,
        .room = rand() % rooms_count};
    game(&p, map, items, rooms_count, backup);
    for (int i = 0; i < rooms_count; i++)
        free(map[i].neigh);
    free(map);
    free(items);
}

bool is_won(item_t *items, room_t *rooms, int rooms_count)
{
    for (int i = 0; i < rooms_count; i++)
    {
        if (rooms[i].k > 0)
            if (items[rooms[i].items[0]].dest != i)
                return false;
        if (rooms[i].k > 1)
            if (items[rooms[i].items[1]].dest != i)
                return false;
    }
    return true;
}