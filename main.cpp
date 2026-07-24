#include "entities.h" 

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

namespace fs = std::filesystem;

using KickTable = std::map<std::pair<int, int>, std::vector<SDL_Point>>;
using KickList = std::map<char, KickTable>;

const int fps = 120;

void draw_grid(SDL_Renderer* ren, const int g_size, const int w, const int h, const int grid_x, const int grid_y);
std::vector<Mix_Music*> load_music();
std::map<char, SDL_Texture*> load_textures(SDL_Renderer* ren);
void draw_matrix(SDL_Renderer* ren, const Tetro& block, int tetro_x, int tetro_y, int ghost_y, int start_x, int start_y, int g_size, char color_key, const std::map<char, SDL_Texture*>& textures);
char rand_choice(const std::vector<char>& vec);
bool collide(const Tetro& block, int off_x, int off_y, int play_grid_x, int play_grid_y, const std::vector<std::vector<char>>& board);
void clear_line(std::vector<std::vector<char>>& board, int play_x, int play_y);
void text(SDL_Renderer* ren, TTF_Font* font, std::string txt, int x, int y);

const std::string BASE_MUSIC_DIR = "assets/music/";

std::random_device rd;
std::mt19937 g(rd());

int fall_delay = 90;

const int DAS = 120;
const int ARR = 20;
const int SDF = 6;

const Uint8 GHOST_ALPHA = 70;

const SDL_Color FONT_COLOR = {255, 255, 255, 255};

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
    {'J', jlstz_kicks},
    {'L', jlstz_kicks},
    {'S', jlstz_kicks},
    {'T', jlstz_kicks},
    {'Z', jlstz_kicks},
    {'I', i_kicks},
    {'O', {}}
};

std::vector<bool> active_dir = {false, false};
std::vector<bool> das_triggered = {false, false};
std::vector<Uint64> last_move_time = {0, 0};

enum {LEFT, RIGHT};

std::vector<int> directions = {LEFT, RIGHT};

int get_ghost_y(const Tetro& block, int play_grid_x, int play_grid_y, const std::vector<std::vector<char>>& board) {
    int ghost_off_y = 0;
    while (!collide(block, 0, ghost_off_y + 1, play_grid_x, play_grid_y, board)) {
        ghost_off_y++;
    }
    return block.y + ghost_off_y;
}

int main(int argc, char* argv[]) {

    TTF_Init();

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "png format init error" << std::endl;
        return 1;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0){
        std::cerr << "SDL music init error" << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(48000, AUDIO_F32SYS, 2, 4096) < 0){
        std::cerr << "SDL mix open audio failed" << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    int mixer_flag = MIX_INIT_MP3;
    if ((Mix_Init(mixer_flag) & mixer_flag) != mixer_flag) {
        std::cerr << "Mixer failed to load MP3 support: " << Mix_GetError() << "\n";
        Mix_CloseAudio();
        SDL_Quit();
        return -1;
    }

    std::vector<Mix_Music*> playlist = load_music();
    
    if (!playlist.empty()) {
        Mix_PlayMusic(playlist[0], 0);
    } else {
        std::cerr << "Warning: Playlist is empty! No background track will play.\n";
    }

    SDL_DisplayMode DM;
    if (SDL_GetDesktopDisplayMode(0, &DM) != 0) {
        std::cerr << "SDL Get Display Mode Error: " << SDL_GetError() << std::endl;
        for (Mix_Music* music : playlist) if (music) Mix_FreeMusic(music);
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }
    
    TTF_Font* font = TTF_OpenFont("assets/pixel.ttf", 50);
    if (!font){
        std::cerr << "SDL font loading error: " << SDL_GetError() << std::endl;
    }

    TTF_Font* notice = TTF_OpenFont("assets/pixel.ttf", 30);
    if (!font){
        std::cerr << "SDL font loading error: " << SDL_GetError() << std::endl;
    }

    const int W = DM.w;
    const int H = DM.h;

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
    if (!window) {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        for (Mix_Music* music : playlist) if (music) Mix_FreeMusic(music);
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        std::cerr << "Renderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        for (Mix_Music* music : playlist) if (music) Mix_FreeMusic(music);
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    const auto txture = load_textures(ren);

    Tetro cur_tile;
    cur_tile.setup(PLAY_GRID_X);
    char cur_color = rand_choice(colors);

    std::vector<std::vector<char>> board(PLAY_GRID_Y, std::vector<char>(PLAY_GRID_X, 0));

    int grid_w = PLAY_GRID_X * G_SIZE;
    int grid_h = PLAY_GRID_Y * G_SIZE;
    int start_x = W / 2 - grid_w / 2;
    int start_y = H / 2 - grid_h / 2;

    Uint64 last_fall_time = 0;

    bool space_hold = false;
    bool up_hold = false;

    while (running) {
        Uint64 cur_time = SDL_GetTicks64();
        int current_fall_del = fall_delay;

        const Uint8 *state = SDL_GetKeyboardState(NULL);

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
                        int moves_to_make = 0;
                        if (ARR == 0){
                            moves_to_make = PLAY_GRID_X;
                        }
                        else{
                            moves_to_make = time_held / ARR;
                        }
                    
                        if (moves_to_make > 0){
                            for (int _ = 0; _ < moves_to_make; _++){
                                if (dir == LEFT){
                                    if (!collide(cur_tile, -1, 0, PLAY_GRID_X, PLAY_GRID_Y, board)) { cur_tile.x -= 1; break;}
                                    else {break; }
                                }

                                if (dir == RIGHT){
                                    if (!collide(cur_tile, 1, 0, PLAY_GRID_X, PLAY_GRID_Y, board)) { cur_tile.x += 1; break; }
                                    else {break; }
                                }
                            }

                            if (ARR > 0){
                                last_move_time[dir] += moves_to_make * ARR;
                            }
                        }
            
                    }
                }
            } else {
                active_dir[dir] = false;
                das_triggered[dir] = false;
            }
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYUP) {
                auto key = event.key.keysym.sym;
                if (key == SDLK_SPACE){
                    space_hold = false;
                }
                if (key == SDLK_UP){
                    up_hold = false;
                }
            }

            if (event.type == SDL_KEYDOWN) {
                auto key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) {
                    running = false;
                }
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
                        if (!kicked) {
                            cur_tile.rotate(-1);
                        }
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
                    cur_tile.setup(PLAY_GRID_X);
                    cur_color = rand_choice(colors);

                    if (collide(cur_tile, 0, 0, PLAY_GRID_X, PLAY_GRID_Y, board)){
                        board.assign(PLAY_GRID_Y, std::vector<char>(PLAY_GRID_X, 0));
                    }
                }
            }
        }

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

                cur_tile.setup(PLAY_GRID_X);
                cur_color = rand_choice(colors);

                if (collide(cur_tile, 0, 0, PLAY_GRID_X, PLAY_GRID_Y, board)){
                    board.assign(PLAY_GRID_Y, std::vector<char>(PLAY_GRID_X, 0));
                }
            }
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
        draw_matrix(ren, cur_tile, cur_tile.x, cur_tile.y, active_ghost_y, start_x, start_y, G_SIZE, cur_color, txture);

        clear_line(board, PLAY_GRID_X, PLAY_GRID_Y);

        text(ren, font, "Tetris 1.0", 20, 10);
        text(ren, notice, "Tetris C++ with SDL - Henry", W - 300, H - 50);

        SDL_RenderPresent(ren);

        SDL_Delay(1000 / fps);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(window);

    Mix_HaltMusic();
    for (Mix_Music* music : playlist) {
        if (music != nullptr) {
            Mix_FreeMusic(music);
        }
    }
    playlist.clear();

    for (auto& [key, tex] : txture) {
        if (tex) SDL_DestroyTexture(tex);
    }

    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
    TTF_CloseFont(font);
    TTF_Quit();

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

std::vector<Mix_Music*> load_music() {
    std::vector<Mix_Music*> track;

    if (!fs::exists(BASE_MUSIC_DIR) || !fs::is_directory(BASE_MUSIC_DIR)) {
        std::cerr << "Music directory not found: " << BASE_MUSIC_DIR << "\n";
        return track;
    }

    auto count_dirs = fs::directory_iterator(BASE_MUSIC_DIR);
    int num = (int)std::distance(fs::begin(count_dirs), fs::end(count_dirs));

    track.reserve(num);

    auto loop_dirs = fs::directory_iterator(BASE_MUSIC_DIR);
    for (const auto& entry : loop_dirs) {
        if (entry.is_regular_file()) {
            std::string path_str = entry.path().string();
            Mix_Music* music = Mix_LoadMUS(path_str.c_str());
            
            if (music != nullptr) {
                track.push_back(music);
            } else {
                std::cerr << "Failed to load music asset: " << entry.path().filename() 
                          << " Error: " << Mix_GetError() << "\n";
            }
        }
    }

    std::ranges::shuffle(track, g);

    return track;
}

std::map<char, SDL_Texture*> load_textures(SDL_Renderer* ren) {
    std::map<char, SDL_Texture*> textures;
    
    for (const auto& [key, path] : tiles_path) {
        SDL_Texture* tex = IMG_LoadTexture(ren, path.c_str());
        if (!tex) {
            std::cerr << "Failed to load texture: " << path 
                      << " Error: " << IMG_GetError() << "\n";
        } else {
            textures[key] = tex;
        }
    }
    
    return textures;
}

void draw_matrix(SDL_Renderer* ren, const Tetro& block, int tetro_x, int tetro_y, int ghost_y, int start_x, int start_y, int g_size, char color_key, const std::map<char, SDL_Texture*>& textures) {
    auto it = textures.find(color_key);
    if (it == textures.end() || it->second == nullptr) {
        return;
    }

    const auto& matrix = block.get_shape(); 
    SDL_Texture* tex = it->second;

    if (ghost_y >= 0) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(tex, GHOST_ALPHA);

        for (size_t row = 0; row < matrix.size(); ++row) {
            for (size_t col = 0; col < matrix[row].size(); ++col) {
                if (matrix[row][col] != 0) { 
                    SDL_Rect dest;
                    dest.x = start_x + (tetro_x + static_cast<int>(col)) * g_size;
                    dest.y = start_y + (ghost_y + static_cast<int>(row)) * g_size;
                    dest.w = g_size;
                    dest.h = g_size;

                    SDL_RenderCopy(ren, tex, nullptr, &dest);
                }
            }
        }
        
        SDL_SetTextureAlphaMod(tex, 255);
    }

    for (size_t row = 0; row < matrix.size(); ++row) {
        for (size_t col = 0; col < matrix[row].size(); ++col) {
            if (matrix[row][col] != 0) { 
                SDL_Rect dest;
                dest.x = start_x + (tetro_x + static_cast<int>(col)) * g_size;
                dest.y = start_y + (tetro_y + static_cast<int>(row)) * g_size;
                dest.w = g_size;
                dest.h = g_size;

                SDL_RenderCopy(ren, tex, nullptr, &dest);
            }
        }
    }
}

char rand_choice(const std::vector<char>& vec){
    if (vec.empty()) {
        throw std::invalid_argument("Vector is empty!");
    }

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

                if (new_y >= 0 && board[new_y][new_x] != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

void clear_line(std::vector<std::vector<char>>& board, int play_x, int play_y) {
    uint8_t lines_cleared = 0;

    int row = play_y - 1;

    while (row >= 0) {
        bool row_full = true;
        
        for (int col = 0; col < play_x; col++) {
            if (board[row][col] == 0) { 
                row_full = false;
                break;
            }
        }
        if (row_full) {
            lines_cleared += 1;
            board.erase(board.begin() + row);
            board.insert(board.begin(), std::vector<char>(play_x, 0));
        } else {
            row--;
        }
    }
}

void text(SDL_Renderer* ren, TTF_Font* font, std::string txt, int x, int y){
    SDL_Surface* surface = TTF_RenderText_Solid(font, txt.c_str(), FONT_COLOR);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(ren, surface);

    int text_width = surface->w;
    int text_height = surface->h;

    SDL_Rect destRect = { x, y, text_width, text_height };
    SDL_RenderCopy(ren, texture, NULL, &destRect);
}