#include <stdio.h>
#include <SDL2/SDL.h>
#include <time.h>

#include "emulator.h"
#include "helper.h"
#include "machine.h"

#define LEFT_BUTTON    SDLK_LEFT
#define RIGHT_BUTTON   SDLK_RIGHT
#define FIRE_BUTTON    SDLK_SPACE

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

void machine_key_down(MachineState *ms, int sym) {
  printf("key pressed down: %d\n", sym);
  switch (sym) {
    case FIRE_BUTTON:
      ms->inp_ports[0] |= 0x10;
      break;
    case LEFT_BUTTON:
      ms->inp_ports[0] |= 0x20;
      break;
    case RIGHT_BUTTON:
      ms->inp_ports[0] |= 0x40;
      break;
    default:
      break;
  }
}

void machine_key_up(MachineState *ms, int sym) {
  switch (sym) {
    case FIRE_BUTTON:
      ms->inp_ports[0] &= 0xEF;  // clear bit 4
      break;
    case LEFT_BUTTON:
      ms->inp_ports[0] &= 0xDF;  // clear bit 5
      break;
    case RIGHT_BUTTON:
      ms->inp_ports[0] &= 0xBF;  // clear bit 6
      break;
    default:
      break;
  }
}

uint8_t machine_in(MachineState *ms, uint8_t port_num) {
  uint8_t res;
  switch(port_num) {
    case 3: {
      uint16_t x = (ms->shift1 << 8) | ms->shift0;
      res = ((x >> (8 - ms->shift_offset)) & 0xff);
      break;
    }
    default:
      printf("Warning: Unimplemented input port case: %d", port_num);
  }
  return res;
}

void machine_out(MachineState *ms, uint8_t port, uint8_t val) {
  switch(port) {
    case 2:
      ms->shift_offset = val & 0x7;
      break;
    case 4:
      ms->shift0 = ms->shift1;
      ms->shift1 = val;
      break; 
    default:
      printf("Warning: Unimplemented output port case: %d", port);
  }
}

void handle_8080_instruction(EmulatorState *es, MachineState *ms) {
  uint8_t *opcode = &es->memory[es->PC];
  if (*opcode == 0xdb) {        // Machine specific handling for IN instruction
    uint8_t port = opcode[1];
    es->A = machine_in(ms, port);
    es->PC += 2;
  }
  else if (*opcode == 0xd3) {   // Machine specific handling for OUT instruction
    uint8_t port = opcode[1];
    machine_out(ms, port, es->A);
    es->PC += 2;
  }
  else {
    emulate_8080_op(es);
  }
}