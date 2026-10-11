#include "ui.h"
#include <SDL3_image/SDL_image.h>
#include <random>

Button::Button(SDL_Renderer* ren, MIX_Mixer* mixer, float x, float y, float w, float h, 
               const std::string& norm_path, const std::string& hover_path)
    : rect{x, y, w, h} 
{
    btn_tex = IMG_LoadTexture(ren, norm_path.c_str());
    btn_hover_tex = IMG_LoadTexture(ren, hover_path.c_str());
    if (mixer) {
        click_snd = MIX_LoadAudio(mixer, "assets/sound/clicked.wav", true);
        for (int i = 0; i < 4; ++i) {
            std::string path = "assets/sound/" + std::to_string(i) + ".wav";
            MIX_Audio* chunk = MIX_LoadAudio(mixer, path.c_str(), true);
            if (chunk) hover_snds.push_back(chunk);
        }
    }
}

Button::~Button() {
    if (btn_tex) SDL_DestroyTexture(btn_tex);
    if (btn_hover_tex) SDL_DestroyTexture(btn_hover_tex);
    if (click_snd) MIX_DestroyAudio(click_snd);
    for (auto snd : hover_snds) {
        if (snd) MIX_DestroyAudio(snd);
    }
}

void Button::handle_event(const SDL_Event& e, MIX_Mixer* mixer) {
    if (e.type == SDL_EVENT_MOUSE_MOTION || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        float mx = (e.type == SDL_EVENT_MOUSE_MOTION) ? e.motion.x : e.button.x;
        float my = (e.type == SDL_EVENT_MOUSE_MOTION) ? e.motion.y : e.button.y;
        bool in_bounds = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);

        if (in_bounds && !hovered) {
            hovered = true;
            if (!hover_snds.empty() && mixer) {
                static std::mt19937 ui_gen(std::random_device{}());
                std::uniform_int_distribution<size_t> dist(0, hover_snds.size() - 1);
                MIX_PlayAudio(mixer, hover_snds[dist(ui_gen)]);
            }
        } else if (!in_bounds) {
            hovered = false;
        }

        if (in_bounds && e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            if (click_snd && mixer) MIX_PlayAudio(mixer, click_snd);
            clicked = true;
        }
    } else {
        clicked = false;
    }
}

void Button::render(SDL_Renderer* ren) {
    SDL_Texture* active_tex = (hovered && btn_hover_tex) ? btn_hover_tex : btn_tex;
    if (active_tex) {
        SDL_RenderTexture(ren, active_tex, nullptr, &rect);
    }
}

void Button::reset() {
    clicked = false;
    hovered = false;
}

PauseMenu::~PauseMenu() {
    for (auto btn : buttons) delete btn;
    buttons.clear();
}

void PauseMenu::add_button(SDL_Renderer* ren, MIX_Mixer* mixer, float x, float y, float w, float h, 
                          const std::string& btn_path, const std::string& hover_path) {
    buttons.push_back(new Button(ren, mixer, x, y, w, h, btn_path, hover_path));
}

void PauseMenu::handle_event(const SDL_Event& e, MIX_Mixer* mixer) {
    for (auto btn : buttons) {
        btn->handle_event(e, mixer);
    }
}

void PauseMenu::reset_events() {
    for (auto btn : buttons) {
        btn->clicked = false;
    }
}

void PauseMenu::render(SDL_Renderer* ren, float screen_w, float screen_h) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
    SDL_FRect screen_rect = {0.0f, 0.0f, screen_w, screen_h};
    SDL_RenderFillRect(ren, &screen_rect);

    for (auto btn : buttons) {
        btn->render(ren);
    }
}

TextCache::TextCache(TTF_Font* f, SDL_Color c) : font(f), color(c) {}

TextCache::~TextCache() { 
    if (texture) SDL_DestroyTexture(texture); 
}

void TextCache::render(SDL_Renderer* ren, const std::string& new_text, float x, float y) {
    if (new_text != current_text || !texture) {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        current_text = new_text;
        SDL_Surface* surface = TTF_RenderText_Solid(font, current_text.c_str(), 0, color);
        if (surface) {
            width = static_cast<float>(surface->w);
            height = static_cast<float>(surface->h);
            texture = SDL_CreateTextureFromSurface(ren, surface);
            SDL_DestroySurface(surface);
        }
    }
    if (texture) {
        SDL_FRect dest = { x, y, width, height };
        SDL_RenderTexture(ren, texture, nullptr, &dest);
    }
}