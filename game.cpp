#include "game.h"
#include "rules.h"
#include <SDL3_image/SDL_image.h>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace {
    const int FPS = 60;
    const int DAS = 120;
    const int ARR = 20;
    const int SDF = 20;
    const float GHOST_ALPHA = 70.0f / 255.0f;
    const SDL_Color FONT_COLOR = {255, 255, 255, 255};
    const std::string BASE_MUSIC_DIR = "assets/music/";
}

Game::Game() = default;

Game::~Game() {
    for (auto& [_, tex] : textures) {
        if (tex) SDL_DestroyTexture(tex);
    }
    if (music_track) MIX_DestroyTrack(music_track);
    if (music_audio) MIX_DestroyAudio(music_audio);
    if (mixer) MIX_DestroyMixer(mixer);

    title_text.reset();
    score_text.reset();
    lines_text.reset();
    notice_text.reset();
    next_label_text.reset();

    if (font_large) TTF_CloseFont(font_large);
    if (font_small) TTF_CloseFont(font_small);

    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    MIX_Quit();
    TTF_Quit();
    SDL_Quit();
}

bool Game::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return false;
    if (!TTF_Init()) return false;
    if (!MIX_Init()) return false;

    mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (mixer) MIX_SetMixerGain(mixer, volume);

    auto playlist = load_music();
    if (mixer && !playlist.empty()) {
        music_track = MIX_CreateTrack(mixer);
        music_audio = MIX_LoadAudio(mixer, playlist[0].c_str(), false);
        if (music_audio && music_track) {
            MIX_SetTrackAudio(music_track, music_audio);
            MIX_PlayTrack(music_track, 0);
        }
    }

    const SDL_DisplayMode* dm = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    screen_w = dm ? dm->w : 1920;
    screen_h = dm ? dm->h : 1080;

    window = SDL_CreateWindow("My Tetris", screen_w, screen_h, SDL_WINDOW_FULLSCREEN);
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) return false;
    SDL_SetRenderVSync(renderer, 1);

    font_large = TTF_OpenFont("assets/pixel.ttf", 50);
    font_small = TTF_OpenFont("assets/pixel.ttf", 30);
    title_text = std::make_unique<TextCache>(font_large, FONT_COLOR);
    score_text = std::make_unique<TextCache>(font_large, FONT_COLOR);
    lines_text = std::make_unique<TextCache>(font_large, FONT_COLOR);
    notice_text = std::make_unique<TextCache>(font_small, FONT_COLOR);
    next_label_text = std::make_unique<TextCache>(font_small, FONT_COLOR);

    const int play_w = 600;
    const int min_pad = 20;
    int available_h = screen_h - (2 * min_pad);
    play_grid_x = play_w / g_size;
    play_grid_y = available_h / g_size;

    int grid_w = play_grid_x * g_size;
    int grid_h = play_grid_y * g_size;
    start_x = screen_w / 2 - grid_w / 2;
    start_y = screen_h / 2 - grid_h / 2;

    float btn_w = 124.0f, btn_h = 66.0f, btn_gap = 20.0f;
    float center_x = static_cast<float>(screen_w) / 2.0f - btn_w / 2.0f;
    float menu_start_y = static_cast<float>(screen_h) / 2.0f - (btn_h * 2.0f + btn_gap) / 2.0f;

    pause_menu.add_button(renderer, mixer, center_x, menu_start_y, btn_w, btn_h, "assets/button.png", "assets/clicked.png");
    pause_menu.add_button(renderer, mixer, center_x, menu_start_y + btn_h + btn_gap, btn_w, btn_h, "assets/menu.png", "assets/menu_c.png");
    pause_menu.add_button(renderer, mixer, center_x, menu_start_y + (btn_h + btn_gap) * 2.0f, btn_w, btn_h, "assets/quit.png", "assets/quit_c.png");

    textures = load_textures();
    reset_game();
    return true;
}

void Game::reset_game() {
    board.assign(play_grid_y, std::vector<char>(play_grid_x, 0));
    score = 0;
    lines_cleared_total = 0;
    level = 500 / fall_delay;
    cur_tile.setup(play_grid_x);
    cur_color = rand_choice(colors);
    next_tile.setup(play_grid_x);
    next_color = rand_choice(colors);
}

void Game::place_piece() {
    const auto& shape = cur_tile.get_shape();
    for (size_t r = 0; r < shape.size(); ++r) {
        for (size_t c = 0; c < shape[r].size(); ++c) {
            if (shape[r][c] != 0) {
                int bx = cur_tile.x + static_cast<int>(c);
                int by = cur_tile.y + static_cast<int>(r);
                if (by >= 0 && by < play_grid_y && bx >= 0 && bx < play_grid_x) {
                    board[by][bx] = cur_color;
                }
            }
        }
    }

    cur_tile = next_tile;
    cur_color = next_color;
    next_tile.setup(play_grid_x);
    next_color = rand_choice(colors);

    int cleared = clear_line(board, play_grid_x, play_grid_y);
    if (cleared > 0) {
        static const int base_pts[] = {0, 100, 200, 500, 900};
        score += base_pts[cleared] * level * (last_clear + 1);
        lines_cleared_total += cleared;
    }
    last_clear = cleared;

    if (collide(cur_tile, 0, 0, play_grid_x, play_grid_y, board)) {
        reset_game();
    }
}

void Game::update_movement(Uint64 cur_time, const bool* key_state) {
    if (is_paused) {
        active_dir = {false, false};
        das_triggered = {false, false};
        return;
    }

    std::vector<bool> input_state = {(bool)key_state[SDL_SCANCODE_LEFT], (bool)key_state[SDL_SCANCODE_RIGHT]};
    for (int dir : {LEFT, RIGHT}) {
        if (input_state[dir]) {
            int dx = (dir == LEFT) ? -1 : 1;
            if (!active_dir[dir]) {
                active_dir[dir] = true;
                das_triggered[dir] = false;
                last_move_time[dir] = cur_time;
                if (!collide(cur_tile, dx, 0, play_grid_x, play_grid_y, board)) {
                    cur_tile.x += dx;
                }
            } else {
                Uint64 held = cur_time - last_move_time[dir];
                if (!das_triggered[dir] && held >= DAS) {
                    das_triggered[dir] = true;
                    last_move_time[dir] = cur_time;
                    held = 0;
                }
                if (das_triggered[dir]) {
                    int moves = (ARR == 0) ? play_grid_x : static_cast<int>(held / ARR);
                    for (int i = 0; i < moves; ++i) {
                        if (!collide(cur_tile, dx, 0, play_grid_x, play_grid_y, board)) {
                            cur_tile.x += dx;
                        } else break;
                    }
                    if (ARR > 0 && moves > 0) last_move_time[dir] += moves * ARR;
                }
            }
        } else {
            active_dir[dir] = false;
            das_triggered[dir] = false;
        }
    }
}

void Game::update_fall(Uint64 cur_time, int current_fall_delay) {
    if (is_paused) {
        last_fall_time = cur_time;
        return;
    }

    if (cur_time - last_fall_time >= static_cast<Uint64>(current_fall_delay)) {
        if (!collide(cur_tile, 0, 1, play_grid_x, play_grid_y, board)) {
            cur_tile.y += 1;
        } else {
            score += 1;
            place_piece();
        }
        last_fall_time = cur_time;
    }
}

void Game::handle_inputs(const SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT) running = false;

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE && !esc_held) {
        is_paused = !is_paused;
        esc_held = true;
        if (mixer) MIX_SetMixerGain(mixer, is_paused ? volume / 2.0f : norm_vol);
    }
    if (event.type == SDL_EVENT_KEY_UP) {
        if (event.key.key == SDLK_SPACE) space_hold = false;
        if (event.key.key == SDLK_UP) up_hold = false;
        if (event.key.key == SDLK_ESCAPE) esc_held = false;
    }
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_DELETE) {
        running = false;
    }

    if (is_paused) {
        pause_menu.handle_event(event, mixer);
        if (pause_menu.buttons[0]->clicked) {
            reset_game();
            is_paused = false;
        }
        if (pause_menu.buttons[2]->clicked) {
            running = false;
        }
        return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_UP && !up_hold) {
            up_hold = true;
            int current_rot = cur_tile.get_rotation();
            cur_tile.rotate(1);
            int next_rot = cur_tile.get_rotation();

            if (collide(cur_tile, 0, 0, play_grid_x, play_grid_y, board)) {
                bool kicked = false;
                auto piece_it = wall_kicks.find(cur_tile.type);
                if (piece_it != wall_kicks.end()) {
                    auto kick_it = piece_it->second.find({current_rot, next_rot});
                    if (kick_it != piece_it->second.end()) {
                        for (const auto& kick : kick_it->second) {
                            if (!collide(cur_tile, kick.x, -kick.y, play_grid_x, play_grid_y, board)) {
                                cur_tile.x += kick.x;
                                cur_tile.y -= kick.y;
                                kicked = true;
                                break;
                            }
                        }
                    }
                }
                if (!kicked) cur_tile.rotate(-1);
            }
        }

        if (event.key.key == SDLK_SPACE && !space_hold) {
            space_hold = true;
            cur_tile.y = get_ghost_y(cur_tile, play_grid_x, play_grid_y, board);
            score += 3;
            place_piece();
        }
    }
}

void Game::run() {
    SDL_Event event;
    while (running) {
        Uint64 cur_time = SDL_GetTicks();
        const bool* keys = SDL_GetKeyboardState(nullptr);

        if (keys[SDL_SCANCODE_F1] && volume - 0.02f >= 0.0f) {
            volume -= 0.02f;
            if (mixer) MIX_SetMixerGain(mixer, is_paused ? volume / 2.0f : volume);
            norm_vol = volume;
        }
        if (keys[SDL_SCANCODE_F2] && volume + 0.02f <= 1.0f) {
            volume += 0.02f;
            if (mixer) MIX_SetMixerGain(mixer, is_paused ? volume / 2.0f : volume);
            norm_vol = volume;
        }

        int cur_fall_delay = (!is_paused && keys[SDL_SCANCODE_DOWN]) 
                             ? std::max(1, fall_delay / SDF) 
                             : fall_delay;

        update_movement(cur_time, keys);

        while (SDL_PollEvent(&event)) {
            handle_inputs(event);
        }

        update_fall(cur_time, cur_fall_delay);
        render();
        SDL_Delay(1000 / FPS);
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 1, 10, 65, 255);
    SDL_RenderClear(renderer);

    draw_grid();

    for (int r = 0; r < play_grid_y; ++r) {
        for (int c = 0; c < play_grid_x; ++c) {
            if (board[r][c] != 0) {
                auto it = textures.find(board[r][c]);
                if (it != textures.end() && it->second) {
                    SDL_FRect dest = { 
                        static_cast<float>(start_x + c * g_size), 
                        static_cast<float>(start_y + r * g_size), 
                        static_cast<float>(g_size), 
                        static_cast<float>(g_size) 
                    };
                    SDL_RenderTexture(renderer, it->second, nullptr, &dest);
                }
            }
        }
    }

    int ghost_y = get_ghost_y(cur_tile, play_grid_x, play_grid_y, board);
    draw_matrix(cur_tile, cur_tile.x, cur_tile.y, ghost_y, cur_color, next_tile, next_color);

    title_text->render(renderer, "Tetris 2.0", 20.0f, 10.0f);
    score_text->render(renderer, "Score: " + std::to_string(score), 20.0f, 70.0f);
    lines_text->render(renderer, "Line Clears: " + std::to_string(lines_cleared_total), 20.0f, 130.0f);
    notice_text->render(renderer, "Tetris C++ with SDL - Henry", static_cast<float>(screen_w - 300), static_cast<float>(screen_h - 50));

    if (next_piece_bg.w > 0) {
        next_label_text->render(renderer, "Next Piece:", next_piece_bg.x, next_piece_bg.y - 40.0f);
    }

    if (is_paused) {
        pause_menu.render(renderer, static_cast<float>(screen_w), static_cast<float>(screen_h));
    }

    SDL_RenderPresent(renderer);
}

void Game::draw_grid() {
    int grid_w = play_grid_x * g_size;
    int grid_h = play_grid_y * g_size;
    SDL_FRect play_area = { static_cast<float>(start_x), static_cast<float>(start_y), static_cast<float>(grid_w), static_cast<float>(grid_h) };
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &play_area);

    SDL_SetRenderDrawColor(renderer, 31, 31, 31, 255);
    for (int i = 0; i <= play_grid_x; ++i) {
        float x = static_cast<float>(start_x + i * g_size);
        SDL_RenderLine(renderer, x, static_cast<float>(start_y), x, static_cast<float>(start_y + grid_h));
    }
    for (int j = 0; j <= play_grid_y; ++j) {
        float y = static_cast<float>(start_y + j * g_size);
        SDL_RenderLine(renderer, static_cast<float>(start_x), y, static_cast<float>(start_x + grid_w), y);
    }
}

void Game::draw_matrix(const Tetro& block, int tetro_x, int tetro_y, int ghost_y, 
                       char color_key, const Tetro& next, char next_c) {
    auto it = textures.find(color_key);
    if (it == textures.end() || !it->second) return;

    const auto& matrix = block.get_shape(); 
    SDL_Texture* tex = it->second;

    if (ghost_y >= 0) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaModFloat(tex, GHOST_ALPHA);
        for (size_t r = 0; r < matrix.size(); ++r) {
            for (size_t c = 0; c < matrix[r].size(); ++c) {
                if (matrix[r][c] != 0) { 
                    SDL_FRect dest = { 
                        static_cast<float>(start_x + (tetro_x + static_cast<int>(c)) * g_size), 
                        static_cast<float>(start_y + (ghost_y + static_cast<int>(r)) * g_size), 
                        static_cast<float>(g_size), static_cast<float>(g_size) 
                    };
                    SDL_RenderTexture(renderer, tex, nullptr, &dest);
                }
            }
        }
        SDL_SetTextureAlphaModFloat(tex, 1.0f);
    }

    for (size_t r = 0; r < matrix.size(); ++r) {
        for (size_t c = 0; c < matrix[r].size(); ++c) {
            if (matrix[r][c] != 0) { 
                SDL_FRect dest = { 
                    static_cast<float>(start_x + (tetro_x + static_cast<int>(c)) * g_size), 
                    static_cast<float>(start_y + (tetro_y + static_cast<int>(r)) * g_size), 
                    static_cast<float>(g_size), static_cast<float>(g_size) 
                };
                SDL_RenderTexture(renderer, tex, nullptr, &dest);
            }
        }
    }

    auto next_it = textures.find(next_c);
    if (next_it == textures.end() || !next_it->second) return;
    SDL_Texture* next_tex = next_it->second;

    int grid_w = play_grid_x * g_size;
    int starting_draw_x = screen_w / 2 - grid_w / 2 + grid_w + 30;
    int preview_start_y = 100;
    int padding = 20;
    int max_tetro_size = 4;

    next_piece_bg = {
        static_cast<float>(starting_draw_x - padding),
        static_cast<float>(preview_start_y - padding),
        static_cast<float>((max_tetro_size * g_size) + (padding * 2)),
        static_cast<float>((max_tetro_size * g_size) + (padding * 2))
    };

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &next_piece_bg);
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderRect(renderer, &next_piece_bg);

    const auto& next_matrix = next.get_shape();
    for (size_t r = 0; r < next_matrix.size(); ++r) {
        for (size_t c = 0; c < next_matrix[r].size(); ++c) {
            if (next_matrix[r][c] != 0) { 
                SDL_FRect dest = { 
                    static_cast<float>(starting_draw_x + static_cast<int>(c) * g_size), 
                    static_cast<float>(preview_start_y + static_cast<int>(r) * g_size), 
                    static_cast<float>(g_size), static_cast<float>(g_size) 
                };
                SDL_RenderTexture(renderer, next_tex, nullptr, &dest);
            }
        }
    }
}

std::vector<std::string> Game::load_music() {
    std::vector<std::string> tracks;
    if (!fs::exists(BASE_MUSIC_DIR) || !fs::is_directory(BASE_MUSIC_DIR)) return tracks;
    for (const auto& entry : fs::directory_iterator(BASE_MUSIC_DIR)) {
        if (entry.is_regular_file()) tracks.push_back(entry.path().string());
    }
    static std::mt19937 g(std::random_device{}());
    std::ranges::shuffle(tracks, g);
    return tracks;
}

std::map<char, SDL_Texture*> Game::load_textures() {
    std::map<char, SDL_Texture*> texs;
    for (const auto& [key, path] : tiles_path) {
        SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
        if (tex) texs[key] = tex;
    }
    return texs;
}