#ifndef GAME_H
#define GAME_H

#include "renderer.h"
#include "world.h"
#include "registry.h"
#include "mesher.h"

#define GAME_EXIT(error_message)                    \
    do                                         \
    {                                          \
        const char *message = (error_message); \
        fprintf(stderr, message);              \
        exit(EXIT_FAILURE);                    \
    } while (0)

typedef struct Game_T {
    Renderer renderer;
    Registry registry;
    World world;
    Mesher mesher;
} Game_T;

typedef Game_T *Game;

Game createGame();
void destroyGame(Game game);
void gameRun(Game game);

long get_used_memory_kb();

#endif