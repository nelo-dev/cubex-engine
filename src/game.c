#include "game.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>

    long get_used_memory_kb() {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize / 1024; // Convert bytes to KB
        }
        return -1; // Error case
    }

#elif __linux__
    #include <unistd.h>

    long get_used_memory_kb() {
        long rss = 0L;
        FILE* file = fopen("/proc/self/statm", "r");
        if (file == NULL) {
            perror("fopen");
            return -1;
        }

        if (fscanf(file, "%*s %ld", &rss) != 1) {
            perror("fscanf");
            fclose(file);
            return -1;
        }

        fclose(file);

        long page_size_kb = sysconf(_SC_PAGESIZE) / 1024; // Convert bytes to KB
        return rss * page_size_kb;
    }
#else
    #error "Unsupported platform"
#endif

Game createGame()
{
    Game game = (Game) calloc(1, sizeof(Game_T));
    game->registry = createRegistry();
    game->renderer = createRenderer(game->registry, game);
    game->mesher = createMesher(game->registry, game->renderer);
    game->world = createWorld(game->registry, 34, game->renderer, game->mesher);
    
    return game;
}

uint64_t getCurrentTimeMillis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void gameRun(Game game)
{
    const uint64_t tickInterval = 50;
    uint64_t lastTickTime = getCurrentTimeMillis();

    while(!RendererHasFinished(game->renderer))
    {
        uint64_t currentTime = getCurrentTimeMillis();
        uint64_t elapsedTime = currentTime - lastTickTime;

        if (elapsedTime >= tickInterval) {
            WorldTickUpdateInfo worldUpdateInfo = {
                .player_x = game->renderer->camera->camPos.x,
                .player_z = game->renderer->camera->camPos.z,
            };

            worldTickUpdate(game->world, &worldUpdateInfo);

            lastTickTime += tickInterval;
        } else {
            usleep((tickInterval - elapsedTime) * 1000);
        }
    }
}

void destroyGame(Game game)
{
    destroyMesher(game->mesher); 
    destroyWorld(game->world);
    destroyRenderer(game->renderer);
    destroyRegistry(game->registry);
    free(game);
}