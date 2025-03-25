#include "game.h"

#include <stdio.h>

int main()
{
    Game game = createGame();
    gameRun(game);
    destroyGame(game);

    printf("\033[0;32mTerminated!\033[0m\n");
    return 0;
}