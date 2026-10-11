#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <random>

#include "entities.h"
#include "ui.h"

class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();

private:
    void handle_inputs(const SDL_Event& event);
    void update_movement(Uint64 cur_time, const bool* key_state);
    void update_fall(Uint64 cur_time, int current_fall_delay);
    void place_piece();
    void reset_game();
    void render();

    void draw_grid();
    void draw_matrix(const Tetro& block, int tetro_x, int tetro_y, int ghost_y, 
                     char color_key, const Tetro& next, char next_c);
    std::vector<std::string> load_music();
    std::map<char, SDL_Texture*> load_textures();

    int screen_w = 1920;
    int screen_h = 1080;
    const int g_size = 30;
    int play_grid_x = 0;
    int play_grid_y = 0;
    int start_x = 0;
    int start_y = 0;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    MIX_Mixer* mixer = nullptr;
    MIX_Track* music_track = nullptr;
    MIX_Audio* music_audio = nullptr;

    TTF_Font* font_large = nullptr;
    TTF_Font* font_small = nullptr;
    std::unique_ptr<TextCache> title_text;
    std::unique_ptr<TextCache> score_text;
    std::unique_ptr<TextCache> lines_text;
    std::unique_ptr<TextCache> notice_text;
    std::unique_ptr<TextCache> next_label_text;

    std::map<char, SDL_Texture*> textures;
    PauseMenu pause_menu;

    bool running = true;
    bool is_paused = false;
    float volume = 1.0f;
    float norm_vol = 1.0f;

    Tetro cur_tile;
    char cur_color = 'I';
    Tetro next_tile;
    char next_color = 'I';
    std::vector<std::vector<char>> board;

    int fall_delay = 90;
    int level = 5;
    int score = 0;
    int lines_cleared_total = 0;
    int last_clear = 0;
    Uint64 last_fall_time = 0;

    SDL_FRect next_piece_bg{0, 0, 0, 0};

    bool space_hold = false;
    bool up_hold = false;
    bool esc_held = false;

    enum Dir { LEFT = 0, RIGHT = 1 };
    std::vector<bool> active_dir{false, false};
    std::vector<bool> das_triggered{false, false};
    std::vector<Uint64> last_move_time{0, 0};
};