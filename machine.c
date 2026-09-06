#include <stdio.h>
#include <SDL2/SDL.h>

#include "emulator.h"

#define MAX_INSTRUCTIONS 999999

typedef struct MachineState{
  uint8_t shift_offset;
  uint8_t shift0;
  uint8_t shift1;
} MachineState;
MachineState machine_state = {0};

void machine_key_down(int sym) {
  printf("key down: %d\n", sym);
}
void machine_key_up(int sym) {
  printf("key up: %d\n", sym);
}

void read_keypress_events() {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch (event.type) {
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

uint8_t machine_in(uint8_t port) {
  uint8_t res;
  switch(port) {
    case 3:
      uint16_t x = (machine_state.shift1 << 8) | machine_state.shift0;
      res = ((x >> (8 - machine_state.shift_offset)) & 0xff);
      break;
  }
  return res;
};

void machine_out(uint8_t port, uint8_t val) {
  switch(port) {
    case 2:
      machine_state.shift_offset = val & 0x7;
      break;
    case 4:
      machine_state.shift0 = machine_state.shift1;
      machine_state.shift1 = val;
      break; 
  }
};

void start_machine(char filepath[]) {  
  EmulatorState *em_state = init_emulator_state(filepath);

  uint32_t instructions_ran = 0;
  while (instructions_ran < MAX_INSTRUCTIONS) {
    // uint8_t *opcode = &em_state->memory[em_state->PC];
    // if (*opcode == 0xdb) {   // Machine specific handling for IN instruction
    //   uint8_t port = opcode[1];
    //   em_state->A = machine_in(port);
    //   em_state->PC++;
    // }
    // else if (*opcode == 0xd3) {   // Machine specific handling for OUT instruction
    //   uint8_t port = opcode[1];
    //   machine_out(port, em_state->A);
    //   em_state->PC++;
    // }
    // else {
    //   emulate_8080_op(em_state);
    // }
    // instructions_ran++;
    read_keypress_events();
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