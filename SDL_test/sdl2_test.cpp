#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


typedef struct {
    SDL_Rect draw_rect;    // dimensions of button
    struct {
        Uint8 r, g, b, a;
    } colour;

    bool pressed;
} button_t;

static void button_process_event(button_t *btn, const SDL_Event *ev) {
    // react on mouse click within button rectangle by setting 'pressed'
    if (ev->type == SDL_MOUSEBUTTONDOWN) {
        if (ev->button.button == SDL_BUTTON_LEFT &&
            ev->button.x >= btn->draw_rect.x &&
            ev->button.x <= (btn->draw_rect.x + btn->draw_rect.w) &&
            ev->button.y >= btn->draw_rect.y &&
            ev->button.y <= (btn->draw_rect.y + btn->draw_rect.h)) {
            btn->pressed = true;
        }
    }
}

static bool button(SDL_Renderer *r, button_t *btn) {
    // draw button
    SDL_SetRenderDrawColor(r, btn->colour.r, btn->colour.g, btn->colour.b, btn->colour.a);
    SDL_RenderFillRect(r, &btn->draw_rect);

    // if button press detected - reset it so it wouldn't trigger twice
    if (btn->pressed) {
        btn->pressed = false;
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {
    int quit = 0;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = NULL;
    window = SDL_CreateWindow("122121", 350, 150, 800, 500, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "create window failed: %s\n", SDL_GetError());
        return 1;   // 'error' return status is !0. 1 is good enough
    }

    SDL_Renderer* renderer = NULL;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {   // renderer creation may fail too
        fprintf(stderr, "create renderer failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Texture* txt = NULL;

    SDL_Rect rct;
    rct.x = 0;
    rct.y = 0;
    rct.h = 500;
    rct.w = 800;

    // button state - colour and rectangle
    button_t start_button;
    start_button.colour = { 100,100,100,100 };
    start_button.draw_rect = { 128 ,128 ,128 ,128 };

    enum {
        STATE_IN_MENU,
        STATE_IN_GAME,
    } state = STATE_IN_MENU;

    while (!quit) {
        SDL_Event evt;    // no need for new/delete, stack is fine

                          // event loop and draw loop are separate things, don't mix them
        while (SDL_PollEvent(&evt)) {
            // quit on close, window close, or 'escape' key hit
            if (evt.type == SDL_QUIT ||
                (evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_CLOSE) ||
                (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE)) {
                quit = 1;
            }

            // pass event to button
            button_process_event(&start_button, &evt);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        //      SDL_RenderCopy(renderer, txt, NULL, &rct);

        if (state == STATE_IN_MENU) {
            if (button(renderer, &start_button)) {
                printf("start button pressed\n");
                state = STATE_IN_GAME;   // state change - button will not be drawn anymore
            }
        }
        else if (state == STATE_IN_GAME) {
            /* your game logic */
        }

        SDL_RenderPresent(renderer);
    }
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    return 0;
}