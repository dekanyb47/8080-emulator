#include <stdio.h>
#include <SDL2/SDL.h>
#include <time.h>

#include "emulator.h"

#define CONTROL_LEFT    SDLK_LEFT
#define CONTROL_RIGHT   SDLK_RIGHT
#define CONTROL_FIRE    SDLK_SPACE


typedef struct MachineState{
  uint8_t shift_offset;
  uint8_t shift0;
  uint8_t shift1;
  uint8_t *inp_ports;   // numbered 0, 1, 2
  uint8_t *out_ports;   // numbered 2, 3, 5, 6
  uint32_t last_interrupt;
  uint8_t done;
} MachineState;

// TODO: last interrupt
MachineState *init_machine_state() {
  // TODO: some bytes need to be set to always 1.
  uint8_t *inp_ports = malloc(3);
  uint8_t *out_ports = malloc(4);
  MachineState *ms = calloc(sizeof(MachineState), 1);
  if (inp_ports == NULL || out_ports == NULL || ms == NULL) die("malloc");
  
  ms->inp_ports = inp_ports;
  ms->out_ports = out_ports;
  return ms;
}

MachineState *machine_state;  // initialized in the start_machine function

void init_sdl_window() {
  if (!SDL_Init(SDL_INIT_VIDEO)) die("SDL_Init");
  if (SDL_CreateWindow("Intel 8080 Emulator", 0, 0, 500, 500, 0) == NULL) die("SDL_CreateWindow");
}

void machine_key_down(int sym) {
  switch (sym) {
    case CONTROL_FIRE:
      machine_state->inp_ports[0] |= 0x10;
      break;
    case CONTROL_LEFT:
      machine_state->inp_ports[0] |= 0x20;
      break;
    case CONTROL_RIGHT:
      machine_state->inp_ports[0] |= 0x40;
      break;
    default:
      break;
  }
}

void machine_key_up(int sym) {
  switch (sym) {
    case CONTROL_FIRE:
      machine_state->inp_ports[0] &= 0xEF;  // clear bit 4
      break;
    case CONTROL_LEFT:
      machine_state->inp_ports[0] &= 0xDF;  // clear bit 5
      break;
    case CONTROL_RIGHT:
      machine_state->inp_ports[0] &= 0xBF;  // clear bit 6
      break;
    default:
    break;
  }
}

void read_keypress_events() {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        machine_state->done = 1;
        break;
      case SDL_KEYDOWN:
        machine_key_down(event.key.keysym.sym);
        break;
      case SDL_KEYUP:
        machine_key_up(event.key.keysym.sym);
        break;
      default:
        break;
    }
  }
}

uint8_t machine_in(uint8_t port_num) {
  uint8_t res;
  switch(port_num) {
    case 3: {
      uint16_t x = (machine_state->shift1 << 8) | machine_state->shift0;
      res = ((x >> (8 - machine_state->shift_offset)) & 0xff);
      break;
    }
    default:
      printf("Warning: Unimplemented input port case: %d", port_num);
  }
  return res;
}

void machine_out(uint8_t port, uint8_t val) {
  switch(port) {
    case 2:
      machine_state->shift_offset = val & 0x7;
      break;
    case 4:
      machine_state->shift0 = machine_state->shift1;
      machine_state->shift1 = val;
      break; 
    default:
      printf("Warning: Unimplemented output port case: %d", port);
  }
}

// 
void handle_8080_instruction(EmulatorState *em_state) {
  uint8_t *opcode = &em_state->memory[em_state->PC];
  if (*opcode == 0xdb) {        // Machine specific handling for IN instruction
    uint8_t port = opcode[1];
    em_state->A = machine_in(port);
    em_state->PC += 2;
  }
  else if (*opcode == 0xd3) {   // Machine specific handling for OUT instruction
    uint8_t port = opcode[1];
    machine_out(port, em_state->A);
    em_state->PC += 2;
  }
  else {
    emulate_8080_op(em_state);
  }
}

void handle_timed_tasks(EmulatorState *em_state) {
  read_keypress_events();
  if (time(NULL) - machine_state->last_interrupt > 1.0 / 60.0) {
    if (em_state->int_enable) {
      generate_interrupt(em_state, 2);
      machine_state->last_interrupt = time(NULL);
    }
  }
}

void start_machine(char filepath[]) {  
  EmulatorState *em_state = init_emulator_state(filepath);
  machine_state = init_machine_state();
  init_sdl_window();

  while (!machine_state->done) {
    // handle_8080_instruction(em_state);
    handle_timed_tasks(em_state);
  }
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("Please specify the file path to the program you wish to open as the input argument!\n");
    return 1;
  }
  
  start_machine(argv[1]);

  return 0;
}