#include "common.h"
#include "Game.hpp"

void GameLoop(void* arg)
{
    static_cast<Game*>(arg)->game_loop();
}

int main(int argc, char * argv[]) {
    Game game;
    game.init("Sokoban", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, PIXEL_WIDTH, PIXEL_HEIGHT, false);
    emscripten_set_main_loop_arg(GameLoop, &game, 0, 1);
    game.clean();
    return 0;
}