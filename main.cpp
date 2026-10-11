#include "game.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Game game;

    if (!game.init()) {
        std::cerr << "Failed to initialize game subsystems." << std::endl;
        return 1;
    }

    game.run();

    return 0;
}