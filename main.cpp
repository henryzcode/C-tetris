#include <SDL2/SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <functional>

#include "entities.h"

namespace fs = std::filesystem;

using KickTable = std::map<std::pair<int, int>, std::vector<SDL_Point>>;
using KickList = std::map<char, KickTable>;

const int fps = 60;
uint8_t volume = MIX_MAX_VOLUME;
uint8_t norm_vol = volume;

class Button {
private:
    SDL_Texture* btn_tex = nullptr;
    SDL_Texture* btn_hover_tex = nullptr;
    Mix_Chunk* click_snd = nullptr;
    std::vector<Mix_Chunk*> hover_snds;

public:
    SDL_Rect rect{0, 0, 124, 66};
    bool clicked = false;
    bool hovered = false;

    Button(SDL_Renderer* ren, int x, int y, int w, int h, 
           const std::string& norm_path, const std::string& hover_path) 
        : rect{x, y, w, h} 
    {
        btn_tex = IMG_LoadTexture(ren, norm_path.c_str());
        btn_hover_tex = IMG_LoadTexture(ren, hover_path.c_str());
        click_snd = Mix_LoadWAV("assets/sound/clicked.wav");
        
        for (int i = 0; i < 4; ++i) {
            std::string path = "assets/sound/" + std::to_string(i) + ".wav";
            Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
            if (chunk) hover_snds.push_back(chunk);
        }
    }

    ~Button() {
        if (btn_tex) SDL_DestroyTexture(btn_tex);
        if (btn_hover_tex) SDL_DestroyTexture(btn_hover_tex);
        if (click_snd) Mix_FreeChunk(click_snd);
        for (auto snd : hover_snds) {
            if (snd) Mix_FreeChunk(snd);
        }
    }

    void handle_event(const SDL_Event& e) {
        if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN) {
            int mx = e.motion.x;
            int my = e.motion.y;
            bool in_bounds = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);

            if (in_bounds && !hovered) {
                hovered = true;
                if (!hover_snds.empty()) {
                    static std::mt19937 ui_gen(std::random_device{}());
                    std::uniform_int_distribution<size_t> dist(0, hover_snds.size() - 1);
                    auto chunk = hover_snds[dist(ui_gen)];
                    Mix_VolumeChunk(chunk, volume / 2);
                    Mix_PlayChannel(-1, chunk, 0);
                }
            } else if (!in_bounds) {
                hovered = false;
            }

            if (in_bounds && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                Mix_VolumeChunk(click_snd, volume / 2);
                if (click_snd) Mix_PlayChannel(-1, click_snd, 0);
                clicked = true;
            }
        }
        else{
            clicked = false;
        }
    }

    void render(SDL_Renderer* ren) {
        SDL_Texture* active_tex = (hovered && btn_hover_tex) ? btn_hover_tex : btn_tex;
        if (active_tex) {
            SDL_RenderCopy(ren, active_tex, NULL, &rect);
        }
    }

    void reset() {
        clicked = false;
        hovered = false;
    }
};

class PauseMenu {
public:
    std::vector<Button*> buttons;

    ~PauseMenu() {
        for (auto btn : buttons) delete btn;
        buttons.clear();
    }

    void add_button(SDL_Renderer* ren, int x, int y, int w, int h, 
                    const std::string& btn_path, const std::string& hover_path) {
        buttons.push_back(new Button(ren, x, y, w, h, btn_path, hover_path));
    }

    void handle_event(const SDL_Event& e) {
        for (auto btn : buttons) {
            btn->handle_event(e);
        }
    }

    void reset_events() {
        for (auto btn : buttons) {
            btn->clicked = false;
        }
    }

    void render(SDL_Renderer* ren, int screen_w, int screen_h) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
        SDL_Rect screen_rect = {0, 0, screen_w, screen_h};
        SDL_RenderFillRect(ren, &screen_rect);

        for (auto btn : buttons) {
            btn->render(ren);
        }
    }
};

class TextCache {
private:
    SDL_Texture* texture = nullptr;
    std::string current_text = "";
    int width = 0, height = 0;
    TTF_Font* font = nullptr;
    SDL_Color color;

public:
    TextCache(TTF_Font* f, SDL_Color c) : font(f), color(c) {}
    
    ~TextCache() { 
        if (texture) SDL_DestroyTexture(texture); 
    }

    TextCache(const TextCache&) = delete;
    TextCache& operator=(const TextCache&) = delete;

    void render(SDL_Renderer* ren, const std::string& new_text, int x, int y) {
        if (new_text != current_text || !texture) {
            if (texture) {
                SDL_DestroyTexture(texture);
                texture = nullptr;
            }
            current_text = new_text;
            SDL_Surface* surface = TTF_RenderText_Solid(font, current_text.c_str(), color);
            if (surface) {
                width = surface->w;
                height = surface->h;
                texture = SDL_CreateTextureFromSurface(ren, surface);
                SDL_FreeSurface(surface);
            }
        }
        if (texture) {
            SDL_Rect dest = { x, y, width, height };
            SDL_RenderCopy(ren, texture, NULL, &dest);
        }
    }
};

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

void draw_grid(SDL_Renderer* ren, const int g_size, const int w, const int h, const int grid_x, const int grid_y);
std::vector<std::string> load_music();
std::map<char, SDL_Texture*> load_textures(SDL_Renderer* ren);
void draw_matrix(SDL_Renderer* ren, const Tetro& block, int tetro_x, int tetro_y, int ghost_y, int start_x, int start_y, int g_size, char color_key, const std::map<char, SDL_Texture*>& textures, int grid_x, int grid_y, int w, const Tetro& next, char next_color, SDL_Rect* next_bg);
char rand_choice(const std::vector<char>& vec);
bool collide(const Tetro& block, int off_x, int off_y, int play_grid_x, int play_grid_y, const std::vector<std::vector<char>>& board);
void clear_line(std::vector<std::vector<char>>& board, int play_x, int play_y);

const std::string BASE_MUSIC_DIR = "assets/music/";

std::random_device rd;
std::mt19937 g(rd());

int fall_delay = 90;
int level = 500 / fall_delay;
int lines_cleared_total = 0;
int score = 0;

const int DAS = 120;
const int ARR = 20;
const int SDF = 10;

const Uint8 GHOST_ALPHA = 70;
const SDL_Color FONT_COLOR = {255, 255, 255, 255};

int last_clear = 0;

std::vector<bool> active_dir = {false, false};
std::vector<bool> das_triggered = {false, false};
std::vector<Uint64> last_move_time = {0, 0};

enum {LEFT, RIGHT};
std::vector<int> directions = {LEFT, RIGHT};

Uint32 MUSIC_END = 0;

void callback() {
    SDL_Event event;
    SDL_zero(event);
    event.type = MUSIC_END;
    SDL_PushEvent(&event);
}

int get_ghost_y(const Tetro& block, int play_grid_x, int play_grid_y, const std::vector<std::vector<char>>& board) {
    int ghost_off_y = 0;
    while (!collide(block, 0, ghost_off_y + 1, play_grid_x, play_grid_y, board)) {
        ghost_off_y++;
    }
    return block.y + ghost_off_y;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return 1;
    if (TTF_Init() != 0) return 1;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return 1;
    if (Mix_OpenAudio(48000, AUDIO_F32SYS, 2, 1024) < 0) return 1;

    int mixer_flag = MIX_INIT_MP3;
    if ((Mix_Init(mixer_flag) & mixer_flag) != mixer_flag) {}

    std::vector<std::string> playlist = load_music();
    Mix_Music* current_music = nullptr;

    MUSIC_END = SDL_RegisterEvents(1);
    Mix_HookMusicFinished(callback);

    Mix_VolumeMusic(volume);
    
    int cur_index = 0;
    if (!playlist.empty()) {
        current_music = Mix_LoadMUS(playlist[cur_index].c_str());
        if (current_music) Mix_FadeInMusic(current_music, 0, 10000);
    }

    SDL_DisplayMode DM;
    SDL_GetDesktopDisplayMode(0, &DM);

    TTF_Font* font = TTF_OpenFont("assets/pixel.ttf", 50);
    TTF_Font* notice = TTF_OpenFont("assets/pixel.ttf", 30);
    
    TextCache titleText(font, FONT_COLOR);
    TextCache scoreText(font, FONT_COLOR);
    TextCache linesText(font, FONT_COLOR);
    TextCache noticeText(notice, FONT_COLOR);
    TextCache nextLabelText(notice, FONT_COLOR);

    const int W = DM.w;
    const int H = DM.h;

    SDL_Rect next_piece_bg = {0, 0, 0, 0};

    const int G_SIZE = 30; 
    const int PLAY_W = 600;
    const int MIN_PADDING = 20; 
    int available_height = H - (2 * MIN_PADDING); 

    const int PLAY_GRID_X = PLAY_W / G_SIZE;
    const int PLAY_GRID_Y = available_height / G_SIZE;

    SDL_Window* window = SDL_CreateWindow(
        "My Tetris",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    SDL_Event event;

    SDL_Renderer* ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    bool is_paused = false;
    bool running = true;

    PauseMenu menu;
    int btn_w = 124, btn_h = 66, btn_gap = 20;
    int center_x = W / 2 - btn_w / 2;
    int start_y_ = H / 2 - (btn_h * 2 + btn_gap) / 2;

    menu.add_button(ren, center_x, start_y_, btn_w, btn_h, "assets/button.png", "assets/clicked.png");
    menu.add_button(ren, center_x, start_y_ + btn_h + btn_gap, btn_w, btn_h, "assets/menu.png", "assets/menu_c.png");
    menu.add_button(ren, center_x, start_y_ + btn_h + btn_gap + btn_h + btn_gap, btn_w, btn_h, "assets/quit.png", "assets/quit_c.png");

    const auto txture = load_textures(ren);

    Tetro cur_tile;
    cur_tile.setup(PLAY_GRID_X);
    char cur_color = rand_choice(colors);

    Tetro next_tile;
    next_tile.setup(PLAY_GRID_X);
    char next_color = rand_choice(colors);

    std::vector<std::vector<char>> board(PLAY_GRID_Y, std::vector<char>(PLAY_GRID_X, 0));

    int grid_w = PLAY_GRID_X * G_SIZE;
    int grid_h = PLAY_GRID_Y * G_SIZE;
    int start_x = W / 2 - grid_w / 2;
    int start_y = H / 2 - grid_h / 2;

    Uint64 last_fall_time = 0;
    bool space_hold = false;
    bool up_hold = false;

    bool esc_held = false;

    while (running) {
        Uint64 cur_time = SDL_GetTicks64();
        int current_fall_del = fall_delay;
        const Uint8 *state = SDL_GetKeyboardState(NULL);

        if (state[SDL_SCANCODE_F1] && volume - 2 >= 0){
            volume -= 2;
            Mix_VolumeMusic(volume);
            if (is_paused){
                if (volume > MIX_MAX_VOLUME / 2){
                    volume = MIX_MAX_VOLUME / 2;
                }
                norm_vol = volume * 2;
            }
            else{
                norm_vol = volume;
            }
        }
        if (state[SDL_SCANCODE_F2] && volume + 2 <= MIX_MAX_VOLUME){
            volume += 2;
            Mix_VolumeMusic(volume);
            if (is_paused){
                if (volume > MIX_MAX_VOLUME / 2){
                    volume = MIX_MAX_VOLUME / 2;
                }
                norm_vol = volume * 2;
            }
            else{
                norm_vol = volume;
            }
        }

        if (!is_paused) {
            if (state[SDL_SCANCODE_DOWN]){
                current_fall_del = std::max(1, fall_delay / SDF);
            }

            std::vector<bool> input_state = {
                (bool) state[SDL_SCANCODE_LEFT],
                (bool) state[SDL_SCANCODE_RIGHT],
            };

            for (int dir: directions){
                if (input_state[dir]){
                    if (!active_dir[dir]){
                        active_dir[dir] = true;
                        das_triggered[dir] = false;
                        last_move_time[dir] = cur_time;

                        switch (dir){
                            case LEFT:
                                if (!collide(cur_tile, -1, 0, PLAY_GRID_X, PLAY_GRID_Y, board)) { cur_tile.x -= 1; }
                                break;
                            case RIGHT:
                                if (!collide(cur_tile, 1, 0, PLAY_GRID_X, PLAY_GRID_Y, board)) { cur_tile.x += 1; }
                                break;
                        }
                    }
                    else {
                        Uint64 time_held = cur_time - last_move_time[dir];
                        if ((!das_triggered[dir]) && time_held >= DAS){
                            das_triggered[dir] = true;
                            last_move_time[dir] = cur_time;
                            time_held = 0;
                        }

                        if (das_triggered[dir]){
                            int moves_to_make = (ARR == 0) ? PLAY_GRID_X : time_held / ARR;
                        
                            if (moves_to_make > 0){
                                for (int _ = 0; _ < moves_to_make; _++){
                                    if (dir == LEFT){
                                        if (!collide(cur_tile, -1, 0, PLAY_GRID_X, PLAY_GRID_Y, board)) { cur_tile.x -= 1; }
                                        else { break; }
                                    }
                                    if (dir == RIGHT){
                                        if (!collide(cur_tile, 1, 0, PLAY_GRID_X, PLAY_GRID_Y, board)) { cur_tile.x += 1; }
                                        else { break; }
                                    }
                                }
                                if (ARR > 0) last_move_time[dir] += moves_to_make * ARR;
                            }
                        }
                    }
                } else {
                    active_dir[dir] = false;
                    das_triggered[dir] = false;
                }
            }
        } else {
            for (int dir: directions) {
                active_dir[dir] = false;
                das_triggered[dir] = false;
            }
        }

        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT) running = false;

            if (event.key.keysym.sym == SDLK_ESCAPE && (!esc_held)) {
                is_paused = !is_paused;
                esc_held = true;
                if (is_paused){
                    Mix_VolumeMusic(volume / 2);
                }
                else{
                    Mix_VolumeMusic(norm_vol);
                }
            }

            if (is_paused) {
                menu.handle_event(event);

                if (menu.buttons[0]->clicked) {
                    board.assign(PLAY_GRID_Y, std::vector<char>(PLAY_GRID_X, 0));
                    score = 0;
                    lines_cleared_total = 0;
                    cur_tile.setup(PLAY_GRID_X);
                    cur_color = rand_choice(colors);
                    is_paused = false; 
                    next_tile.setup(PLAY_GRID_X);
                    next_color = rand_choice(colors);
                }

                if (menu.buttons[1]->clicked) {
                }

                if (menu.buttons[2]->clicked){
                    running = false;
                }
            }

            if (event.type == MUSIC_END && !playlist.empty()){
                if (current_music) Mix_FreeMusic(current_music);
                
                cur_index = (cur_index + 1) % playlist.size();
                if (cur_index == 0) std::ranges::shuffle(playlist, g);

                current_music = Mix_LoadMUS(playlist[cur_index].c_str());
                if (current_music) Mix_FadeInMusic(current_music, 0, 10000);
            }

            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_SPACE) space_hold = false;
                if (event.key.keysym.sym == SDLK_UP) up_hold = false;
                if (event.key.keysym.sym == SDLK_ESCAPE) esc_held = false;
            }

            if (event.key.keysym.sym == SDLK_DELETE){
                running = false;
            }

            if (!is_paused) {
                if (event.type == SDL_KEYDOWN) {
                    auto key = event.key.keysym.sym;

                    if (key == SDLK_UP && (!up_hold)){
                        up_hold = true;
                        int current_rot = cur_tile.get_rotation();
                        cur_tile.rotate(1);
                        int next_rot = cur_tile.get_rotation();

                        if (collide(cur_tile, 0, 0, PLAY_GRID_X, PLAY_GRID_Y, board)) {
                            bool kicked = false;
                            auto piece_it = wall_kicks.find(cur_tile.type);
                            if (piece_it != wall_kicks.end()) {
                                auto transition = std::make_pair(current_rot, next_rot);
                                auto kick_it = piece_it->second.find(transition);
                                
                                if (kick_it != piece_it->second.end()) {
                                    for (const auto& kick : kick_it->second) {
                                        if (!collide(cur_tile, kick.x, -kick.y, PLAY_GRID_X, PLAY_GRID_Y, board)) {
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
                    
                    if (key == SDLK_SPACE && (!space_hold)){
                        space_hold = true;
                        cur_tile.y = get_ghost_y(cur_tile, PLAY_GRID_X, PLAY_GRID_Y, board);
                        const auto& shape = cur_tile.get_shape();

                        for (size_t r = 0; r < shape.size(); ++r) {
                            for (size_t c = 0; c < shape[r].size(); ++c) {
                                if (shape[r][c] != 0) {
                                    int b_x = cur_tile.x + c;
                                    int b_y = cur_tile.y + r;
                                    if (b_y >= 0 && b_y < PLAY_GRID_Y && b_x >= 0 && b_x < PLAY_GRID_X) {
                                        board[b_y][b_x] = cur_color;
                                    }
                                }
                            }
                        }
                        
                        cur_tile = next_tile;
                        cur_color = next_color;
                        next_tile.setup(PLAY_GRID_X);
                        next_color = rand_choice(colors);
                        score += 3;

                        if (collide(cur_tile, 0, 0, PLAY_GRID_X, PLAY_GRID_Y, board)){
                            board.assign(PLAY_GRID_Y, std::vector<char>(PLAY_GRID_X, 0));
                            score = 0;
                            lines_cleared_total = 0;
                        }
                    }
                }
            }
        }

        if (!is_paused) {
            if (cur_time - last_fall_time >= current_fall_del) {
                if (!collide(cur_tile, 0, 1, PLAY_GRID_X, PLAY_GRID_Y, board)) {
                    cur_tile.y += 1;
                } else {
                    const auto& shape = cur_tile.get_shape();
                    for (size_t r = 0; r < shape.size(); ++r) {
                        for (size_t c = 0; c < shape[r].size(); ++c) {
                            if (shape[r][c] != 0) {
                                int b_x = cur_tile.x + c;
                                int b_y = cur_tile.y + r;
                                if (b_y >= 0 && b_y < PLAY_GRID_Y && b_x >= 0 && b_x < PLAY_GRID_X) {
                                    board[b_y][b_x] = cur_color;
                                }
                            }
                        }
                    }

                    cur_tile = next_tile;
                    cur_color = next_color;
                    next_tile.setup(PLAY_GRID_X);
                    next_color = rand_choice(colors);
                    score += 1;

                    if (collide(cur_tile, 0, 0, PLAY_GRID_X, PLAY_GRID_Y, board)){
                        board.assign(PLAY_GRID_Y, std::vector<char>(PLAY_GRID_X, 0));
                        score = 0;
                        lines_cleared_total = 0;
                    }
                }
                last_fall_time = cur_time;
            }
        } else {
            last_fall_time = cur_time; 
        }

        SDL_SetRenderDrawColor(ren, 1, 10, 65, 255);
        SDL_RenderClear(ren);

        draw_grid(ren, G_SIZE, W, H, PLAY_GRID_X, PLAY_GRID_Y);

        for (int r = 0; r < PLAY_GRID_Y; ++r) {
            for (int c = 0; c < PLAY_GRID_X; ++c) {
                if (board[r][c] != 0) {
                    auto it = txture.find(board[r][c]);
                    if (it != txture.end() && it->second != nullptr) {
                        SDL_Rect dest = { start_x + c * G_SIZE, start_y + r * G_SIZE, G_SIZE, G_SIZE };
                        SDL_RenderCopy(ren, it->second, nullptr, &dest);
                    }
                }
            }
        }
        
        int active_ghost_y = get_ghost_y(cur_tile, PLAY_GRID_X, PLAY_GRID_Y, board);
        draw_matrix(ren, cur_tile, cur_tile.x, cur_tile.y, active_ghost_y, start_x, start_y, G_SIZE, cur_color, txture, PLAY_GRID_X, PLAY_GRID_Y, W, next_tile, next_color, &next_piece_bg);

        clear_line(board, PLAY_GRID_X, PLAY_GRID_Y);

        titleText.render(ren, "Tetris 1.0", 20, 10);
        scoreText.render(ren, "Score: " + std::to_string(score), 20, 70);
        linesText.render(ren, "Line Clears: " + std::to_string(lines_cleared_total), 20, 130);
        noticeText.render(ren, "Tetris C++ with SDL - Henry", W - 300, H - 50);

        if (next_piece_bg.w > 0) {
            nextLabelText.render(ren, "Next Piece:", next_piece_bg.x, next_piece_bg.y - 40);
        }

        if (is_paused) {
            menu.render(ren, W, H);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(1000 / fps);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(window);

    Mix_HaltMusic();
    if (current_music) Mix_FreeMusic(current_music);

    for (auto& [key, tex] : txture) {
        if (tex) SDL_DestroyTexture(tex);
    }

    Mix_CloseAudio();
    Mix_Quit();
    TTF_CloseFont(font);
    TTF_CloseFont(notice);
    TTF_Quit();
    SDL_Quit();

    return 0;
}

void draw_grid(SDL_Renderer* ren, const int g_size, const int w, const int h, const int grid_x, const int grid_y) {
    int grid_w = grid_x * g_size;
    int grid_h = grid_y * g_size;

    int start_x = w / 2 - grid_w / 2;
    int start_y = h / 2 - grid_h / 2;

    SDL_Rect play_area = { start_x, start_y, grid_w, grid_h };
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderFillRect(ren, &play_area);
    
    SDL_SetRenderDrawColor(ren, 31, 31, 31, 255);
    for (int i = 0; i <= grid_x; ++i) {
        int x = start_x + i * g_size;
        SDL_RenderDrawLine(ren, x, start_y, x, start_y + grid_h);
    }
    for (int j = 0; j <= grid_y; ++j) {
        int y = start_y + j * g_size;
        SDL_RenderDrawLine(ren, start_x, y, start_x + grid_w, y);
    }
}

std::vector<std::string> load_music() {
    std::vector<std::string> tracks;
    if (!fs::exists(BASE_MUSIC_DIR) || !fs::is_directory(BASE_MUSIC_DIR)) return tracks;

    for (const auto& entry : fs::directory_iterator(BASE_MUSIC_DIR)) {
        if (entry.is_regular_file()) {
            tracks.push_back(entry.path().string());
        }
    }
    std::ranges::shuffle(tracks, g);
    return tracks;
}

std::map<char, SDL_Texture*> load_textures(SDL_Renderer* ren) {
    std::map<char, SDL_Texture*> textures;
    for (const auto& [key, path] : tiles_path) {
        SDL_Texture* tex = IMG_LoadTexture(ren, path.c_str());
        if (tex) textures[key] = tex;
    }
    return textures;
}

void draw_matrix(SDL_Renderer* ren, const Tetro& block, int tetro_x, int tetro_y, int ghost_y, int start_x, int start_y, int g_size, char color_key, const std::map<char, SDL_Texture*>& textures, int grid_x, int grid_y, int w, const Tetro& next, char next_color, SDL_Rect* next_bg) {
    auto it = textures.find(color_key);
    if (it == textures.end() || it->second == nullptr) return;

    const auto& matrix = block.get_shape(); 
    const auto& next_matrix = next.get_shape(); 
    SDL_Texture* tex = it->second;

    int grid_w = grid_x * g_size;
    int starting_draw_x = w / 2 - grid_w / 2 + grid_w + 30;

    if (ghost_y >= 0) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(tex, GHOST_ALPHA);

        for (size_t row = 0; row < matrix.size(); ++row) {
            for (size_t col = 0; col < matrix[row].size(); ++col) {
                if (matrix[row][col] != 0) { 
                    SDL_Rect dest = { start_x + (tetro_x + static_cast<int>(col)) * g_size, 
                                      start_y + (ghost_y + static_cast<int>(row)) * g_size, 
                                      g_size, g_size };
                    SDL_RenderCopy(ren, tex, nullptr, &dest);
                }
            }
        }
        SDL_SetTextureAlphaMod(tex, 255);
    }

    for (size_t row = 0; row < matrix.size(); ++row) {
        for (size_t col = 0; col < matrix[row].size(); ++col) {
            if (matrix[row][col] != 0) { 
                SDL_Rect dest = { start_x + (tetro_x + static_cast<int>(col)) * g_size, 
                                  start_y + (tetro_y + static_cast<int>(row)) * g_size, 
                                  g_size, g_size };
                SDL_RenderCopy(ren, tex, nullptr, &dest);
            }
        }
    }

    it = textures.find(next_color);
    if (it == textures.end() || it->second == nullptr) return;
    tex = it->second;

    int preview_start_y = 100;
    
    int padding = 20;
    int max_tetro_size = 4;
    
    SDL_Rect bg_rect = {
        starting_draw_x - padding,
        preview_start_y - padding,
        (max_tetro_size * g_size) + (padding * 2),
        (max_tetro_size * g_size) + (padding * 2)
    };

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderFillRect(ren, &bg_rect);

    SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
    SDL_RenderDrawRect(ren, &bg_rect);
    
    if (next_bg) {
        *next_bg = bg_rect;
    }

    for (size_t row = 0; row < next_matrix.size(); ++row) {
        for (size_t col = 0; col < next_matrix[row].size(); ++col) {
            if (next_matrix[row][col] != 0) { 
                SDL_Rect dest = { 
                    starting_draw_x + static_cast<int>(col) * g_size, 
                    preview_start_y + static_cast<int>(row) * g_size, 
                    g_size, g_size 
                };
                SDL_RenderCopy(ren, tex, nullptr, &dest);
            }
        }
    }
}

char rand_choice(const std::vector<char>& vec){
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
                int new_x = block.x + col + off_x;
                int new_y = block.y + row + off_y;

                if (new_x < 0 || new_x >= play_grid_x) return true;
                if (new_y >= play_grid_y) return true; 
                if (new_y >= 0 && board[new_y][new_x] != 0) return true;
            }
        }
    }
    return false;
}

void clear_line(std::vector<std::vector<char>>& board, int play_x, int play_y) {
    uint8_t lines_cleared = 0;
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

    if (lines_cleared > 0){
        if (lines_cleared == 1) score += 100 * level * (last_clear + 1);
        else if (lines_cleared == 2) score += 200 * level * (last_clear + 1);
        else if (lines_cleared == 3) score += 500 * level * (last_clear + 1);
        else if (lines_cleared == 4) score += 900 * level * (last_clear + 1);
        lines_cleared_total += lines_cleared;
    }

    last_clear = lines_cleared;
}