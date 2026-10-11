#include "rules.h"
#include <random>
#include <stdexcept>
#include <algorithm>

const KickTable jlstz_kicks = {
    {{0, 1}, {{0, 0}, {-1, 0}, {-1,  1}, {0, -2}, {-1, -2}}},
    {{1, 0}, {{0, 0}, { 1, 0}, { 1, -1}, {0,  2}, { 1,  2}}},
    {{1, 2}, {{0, 0}, { 1, 0}, { 1, -1}, {0,  2}, { 1,  2}}},
    {{2, 1}, {{0, 0}, {-1, 0}, {-1,  1}, {0, -2}, {-1, -2}}},
    {{2, 3}, {{0, 0}, { 1, 0}, { 1,  1}, {0, -2}, { 1, -2}}},
    {{3, 2}, {{0, 0}, {-1, 0}, {-1, -1}, {0,  2}, {-1,  2}}},
    {{3, 0}, {{0, 0}, {-1, 0}, {-1, -1}, {0,  2}, {-1,  2}}},
    {{0, 3}, {{0, 0}, { 1, 0}, { 1,  1}, {0, -2}, { 1, -2}}}
};

const KickTable i_kicks = {
    {{0, 1}, {{0, 0}, {-2, 0}, { 1,  0}, {-2, -1}, { 1,  2}}},
    {{1, 0}, {{0, 0}, { 2, 0}, {-1,  0}, { 2,  1}, {-1, -2}}},
    {{1, 2}, {{0, 0}, {-1, 0}, { 2,  0}, {-1,  2}, { 2, -1}}},
    {{2, 1}, {{0, 0}, { 1, 0}, {-2,  0}, { 1, -2}, {-2,  1}}},
    {{2, 3}, {{0, 0}, { 2, 0}, {-1,  0}, { 2,  1}, {-1, -2}}},
    {{3, 2}, {{0, 0}, {-2, 0}, { 1,  0}, {-2, -1}, { 1,  2}}},
    {{3, 0}, {{0, 0}, { 1, 0}, {-2,  0}, { 1, -2}, {-2,  1}}},
    {{0, 3}, {{0, 0}, {-1, 0}, { 2,  0}, {-1,  2}, { 2, -1}}}
};

const KickList wall_kicks = {
    {'J', jlstz_kicks}, {'L', jlstz_kicks}, {'S', jlstz_kicks},
    {'T', jlstz_kicks}, {'Z', jlstz_kicks}, {'I', i_kicks}, {'O', {}}
};

char rand_choice(const std::vector<char>& vec) {
    if (vec.empty()) throw std::invalid_argument("Vector is empty!");
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, vec.size() - 1);
    return vec[dist(gen)];
}

bool collide(const Tetro& block, int off_x, int off_y, int play_grid_x, int play_grid_y, const std::vector<std::vector<char>>& board) {
    const auto& shape = block.get_shape();
    for (size_t row = 0; row < shape.size(); row++) {
        for (size_t col = 0; col < shape[row].size(); col++) {
            if (shape[row][col] != 0) {
                int new_x = block.x + static_cast<int>(col) + off_x;
                int new_y = block.y + static_cast<int>(row) + off_y;

                if (new_x < 0 || new_x >= play_grid_x) return true;
                if (new_y >= play_grid_y) return true; 
                if (new_y >= 0 && board[new_y][new_x] != 0) return true;
            }
        }
    }
    return false;
}

int get_ghost_y(const Tetro& block, int play_grid_x, int play_grid_y, const std::vector<std::vector<char>>& board) {
    int ghost_off_y = 0;
    while (!collide(block, 0, ghost_off_y + 1, play_grid_x, play_grid_y, board)) {
        ghost_off_y++;
    }
    return block.y + ghost_off_y;
}

int clear_line(std::vector<std::vector<char>>& board, int play_x, int play_y) {
    int lines_cleared = 0;
    int insert_row = play_y - 1;
    
    for (int row = play_y - 1; row >= 0; row--) {
        bool row_full = true;
        bool row_empty = true;
        
        for (int col = 0; col < play_x; col++) {
            if (board[row][col] == 0) row_full = false;
            else row_empty = false;
        }
        
        if (row_full) {
            lines_cleared++;
        } else {
            if (insert_row != row) board[insert_row] = board[row];
            insert_row--;
        }
        if (row_empty && insert_row == row) break; 
    }
    
    while (insert_row >= 0) {
        std::fill(board[insert_row].begin(), board[insert_row].end(), 0);
        insert_row--;
    }

    return lines_cleared;
}