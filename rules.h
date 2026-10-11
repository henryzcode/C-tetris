#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <map>
#include <utility>
#include "entities.h"

using KickTable = std::map<std::pair<int, int>, std::vector<SDL_Point>>;
using KickList = std::map<char, KickTable>;

extern const KickList wall_kicks;

char rand_choice(const std::vector<char>& vec);
bool collide(const Tetro& block, int off_x, int off_y, int play_grid_x, int play_grid_y, const std::vector<std::vector<char>>& board);
int get_ghost_y(const Tetro& block, int play_grid_x, int play_grid_y, const std::vector<std::vector<char>>& board);
int clear_line(std::vector<std::vector<char>>& board, int play_x, int play_y);