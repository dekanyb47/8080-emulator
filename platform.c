#include <SDL2/SDL.h>
#include <time.h>

#include "emulator.h"
#include "helper.h"
#include "machine.h"

typedef struct PlatformState {
  SDL_Window *win;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
} PlatformState;

PlatformState *init_platform_state() {
  PlatformState *platform_state = malloc(sizeof(PlatformState));
  if (platform_state == NULL) die("malloc");

  // TODO: Error: can't initialize SDL
   if (!SDL_Init(SDL_INIT_VIDEO)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
      die("SDL_Init");
    }

  // TODO: warning on this line
  SDL_Window *win = SDL_CreateWindow("Intel 8080 Emulator", 0, 0, 500, 500, 0) == NULL;
  SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_TARGETTEXTURE);  
  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 300, 300);

  platform_state->win = win;
  platform_state->renderer = renderer;
  platform_state->texture = texture;

  return platform_state;
}

void poll_events(MachineState *ms) {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        ms->done = 1;
        break;
      case SDL_KEYDOWN:
        machine_key_down(ms, event.key.keysym.sym);
        break;
      case SDL_KEYUP:
        machine_key_up(ms, event.key.keysym.sym);
        break;
      default:
        break;
    }
  }
}

void handle_timed_tasks(EmulatorState *es, MachineState *ms, PlatformState *ps) {
  poll_events(ms);

  // temporary lines for debugging SDL
  if (!SDL_SetRenderTarget(ps->renderer, ps->texture)) die("SDL_SetRenderTarget");
  if (!SDL_SetRenderDrawColor(ps->renderer, 0xff, 0, 0, 0)) die("SDL_SetRenderDrawColor");
  
  if (time(NULL) - ms->last_interrupt > 1.0 / 60.0) {
    if (es->int_enable) {
      generate_interrupt(es, 2);
      ms->last_interrupt = time(NULL);
    }
  }
}

void init_program(char game_filepath[], EmulatorState **es, MachineState **ms, PlatformState **ps) {  
  *es = init_emulator_state(game_filepath);
  *ms = init_machine_state();
  *ps = init_platform_state();
}

void start_main_loop(EmulatorState *es, MachineState *ms, PlatformState *ps) {
  // TODO: add a delay to help with performance
  while (!ms->done) {
    handle_8080_instruction(es, ms);
    handle_timed_tasks(es, ms, ps);
  }
}

// TODO: free memory, close window, etc.
void shutdown_program() {

}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("Please specify the file path to the program you wish to open as the input argument!\n");
    return 1;
  }
  
  EmulatorState *emulator_state = NULL;
  MachineState *machine_state = NULL;
  PlatformState *platform_state = NULL;
  init_program(argv[1], &emulator_state, &machine_state, &platform_state);

  start_main_loop(emulator_state, machine_state, platform_state);

  shutdown_program();

  return 0;
}
