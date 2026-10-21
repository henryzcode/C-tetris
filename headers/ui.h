#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>

class Button {
private:
    SDL_Texture* btn_tex = nullptr;
    SDL_Texture* btn_hover_tex = nullptr;
    MIX_Audio* click_snd = nullptr;
    std::vector<MIX_Audio*> hover_snds;

public:
    SDL_FRect rect{0, 0, 124, 66};
    bool clicked = false;
    bool hovered = false;

    Button(SDL_Renderer* ren, MIX_Mixer* mixer, float x, float y, float w, float h, 
           const std::string& norm_path, const std::string& hover_path);
    ~Button();

    void handle_event(const SDL_Event& e, MIX_Mixer* mixer);
    void render(SDL_Renderer* ren);
    void reset();
};

class PauseMenu {
public:
    std::vector<Button*> buttons;

    ~PauseMenu();
    void add_button(SDL_Renderer* ren, MIX_Mixer* mixer, float x, float y, float w, float h, 
                    const std::string& btn_path, const std::string& hover_path);
    void handle_event(const SDL_Event& e, MIX_Mixer* mixer);
    void reset_events();
    void render(SDL_Renderer* ren, float screen_w, float screen_h);
};

class TextCache {
private:
    SDL_Texture* texture = nullptr;
    std::string current_text = "";
    float width = 0.0f;
    float height = 0.0f;
    TTF_Font* font = nullptr;
    SDL_Color color;

public:
    TextCache(TTF_Font* f, SDL_Color c);
    ~TextCache();

    TextCache(const TextCache&) = delete;
    TextCache& operator=(const TextCache&) = delete;

    void render(SDL_Renderer* ren, const std::string& new_text, float x, float y);
};