#include <SDL2/SDL.h>
#include <time.h>

#include "emulator.h"
#include "helper.h"
#include "machine.h"

typedef struct PlatformState {
  SDL_Window *win;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint32_t *pixel_buf;
} PlatformState;

#define VRAM_ADDR 0x2400
#define VRAM_LEN 0x1c00

// Converts the 8080's 1bpp vram to a 32bpp rgba formatted pixel buffer that can be used by SDL_UpdateTexture
uint32_t *format_vram(const uint8_t *memory, const uint16_t vram_addr, const uint16_t vram_len) {
  // get VRAM_LEN byte long 1bit per pixel formatted pixel buffer (256 bits per readline)
  uint16_t vram_end_addr = vram_addr + vram_len;
  uint16_t curr_addr = vram_addr;
  uint32_t *res = malloc(vram_len * 32);
  uint32_t res_i = 0;

  if (res == NULL) die("malloc");

  while (curr_addr < vram_end_addr) {
    for (int8_t bitn = 7; bitn >= 0; bitn--) {
      if ((memory[curr_addr] & (1 << bitn)) != 0) res[res_i] = 0xffffffff;    // if the current bit is set
      else res[res_i] = 0x000000ff;
      res_i++;
    }
    
    curr_addr++;
  }

  return res;
}

PlatformState *init_platform_state(EmulatorState *es) {
  PlatformState *platform_state = malloc(sizeof(PlatformState));
  if (platform_state == NULL) die("malloc");

   if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
      die("SDL_Init");
    }

  SDL_Window *win = SDL_CreateWindow("Intel 8080 Emulator", 0, 0, 256, 224, 0);
  platform_state->win = win;

  SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_TARGETTEXTURE);
  platform_state->renderer = renderer;  

  // TODO: screen rotation (anticlockwise 90 degrees)
  // TODO: incorrect rendering to screen
  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 300, 300);
  uint32_t *formatted_vram = format_vram(es->memory, VRAM_ADDR, VRAM_LEN);
  SDL_UpdateTexture(texture, NULL, formatted_vram, 256 * 4);
  platform_state->texture = texture;

  return platform_state;
}

void free_platform_state(PlatformState *ps) {
  SDL_DestroyRenderer(ps->renderer);
  SDL_DestroyTexture(ps->texture);
  SDL_DestroyWindow(ps->win);
  free(ps);
}

void poll_sdl_events(MachineState *ms) {
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

void update_window(PlatformState *ps) {
  // temporary lines for testing
  // SDL_SetRenderTarget(ps->renderer, ps->texture);
  // SDL_RenderClear(ps->renderer);

  SDL_SetRenderTarget(ps->renderer, NULL);
  SDL_RenderCopy(ps->renderer, ps->texture, NULL, NULL);
  SDL_RenderPresent(ps->renderer);
}

// TODO: add support for different types of interrupts
void handle_interrupts(EmulatorState *es, MachineState *ms) {
  if (time(NULL) - ms->last_interrupt > 1.0 / 60.0) {
    if (es->int_enable) {
      generate_interrupt(es, 2);
      ms->last_interrupt = time(NULL);
    }
  }
}

// TODO: move to separate file
void init_program(char game_filepath[], EmulatorState **es, MachineState **ms, PlatformState **ps) {  
  *es = init_emulator_state(game_filepath);
  *ms = init_machine_state();
  *ps = init_platform_state(*es);
}

void start_main_loop(EmulatorState *es, MachineState *ms, PlatformState *ps) {
  // TODO: add internal clock
  while (!ms->done) {
    handle_8080_instruction(es, ms);
    handle_interrupts(es, ms);
    poll_sdl_events(ms);
    update_window(ps);
  }
}

void shutdown_program(EmulatorState *es, MachineState *ms, PlatformState *ps) {
  free_emulator_state(es);
  free_machine_state(ms);
  free_platform_state(ps);
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

  shutdown_program(emulator_state, machine_state, platform_state);
  return 0;
}
