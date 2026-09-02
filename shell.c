#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "disassembler.h"
#include "helper.h"

typedef struct ConditionCodes {
  uint8_t pad : 3;
  uint8_t AC : 1;
  uint8_t CY : 1;
  uint8_t P : 1;
  uint8_t S : 1;
  uint8_t Z : 1;
} ConditionCodes;

typedef struct State8080 {
  uint8_t A;
  uint8_t B;
  uint8_t C;
  uint8_t D;
  uint8_t E;
  uint8_t H;
  uint8_t L;
  uint16_t PC;
  uint16_t SP;
  uint8_t *memory;
  struct ConditionCodes cc;
  uint8_t int_enable;
} State8080;

// TODO: finish and rework reading to memory
State8080 *init_state(char filepath[]) {
  State8080 *state = calloc(1, sizeof(State8080));
  uint8_t *memory = malloc(4096);
  if (state == NULL || memory == NULL) return NULL;

  read_file_to_buf(filepath, &memory, sizeof(memory));

  state->memory = memory;
}

int parity_8bit(uint8_t val) {
  uint8_t curr = val;
  uint8_t set_bytes = 0;
  if (curr >= 0x80) curr -= 0x80; set_bytes++;
  if (curr >= 0x40) curr -= 0x40; set_bytes++;
  if (curr >= 0x20) curr -= 0x20; set_bytes++;
  if (curr >= 0x10) curr -= 0x10; set_bytes++;
  if (curr >= 0x08) curr -= 0x08; set_bytes++;
  if (curr >= 0x04) curr -= 0x04; set_bytes++;
  if (curr >= 0x02) curr -= 0x02; set_bytes++;
  if (curr >= 0x01) curr -= 0x01; set_bytes++;

  return (set_bytes % 2) ? 1 : 0;
}


void call_addr(State8080 *state, const uint16_t addr) {
  uint16_t ret_addr = state->PC+2;
  state->memory[state->SP - 1] = (ret_addr >> 8) & 0xff;
  state->memory[state->SP - 2] = ret_addr & 0xff;
  state->SP -= 2;
  state->PC = addr;
}

void call_return(State8080 *state) {
  state->PC = (state->memory[state->SP + 1] << 8) | state->memory[state->SP];
  state->SP += 2;
}

void direct_load_register_pair(State8080 *state, const unsigned char *opcode, uint8_t *r1, uint8_t *r2) {
  *r1 = opcode[2];
  *r2 = opcode[1];
  state->PC += 2;
}

void push_stack(State8080 *state, uint8_t val1, uint8_t val2) {
  state->memory[state->SP - 1] = val1;
  state->memory[state->SP - 2] = val2;
  state->SP -= 2;
}

void pop_stack(State8080 *state, uint8_t *r1, uint8_t *r2) {
  *r2 = state->memory[state->SP];
  *r1 = state->memory[state->SP + 1];
  state->SP += 2;
}

void move_8bit(uint8_t *dst, uint8_t val) {
  (*dst) = val;
}

void store_A_indirect(State8080 *state, const uint8_t *r1, const uint8_t *r2) {
  const uint16_t offset = ((*r1) << 8) | (*r2);
  state->memory[offset] = state->A;
}

void load_A_indirect(State8080 *state, uint16_t offset) {
  uint8_t val = state->memory[offset];
  state->A = val;
}

void increment_8bit_register_pair(uint8_t *r1, uint8_t *r2) {
  (*r2) += 1;
  if ((*r2) == 0)
    (*r1)++;
}

void increment_8bit_val(State8080 *state, uint8_t *r) {
  uint16_t res = (uint16_t)(*r) + 1;
  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) != 0);
  state->cc.P = parity_8bit(res & 0xff);

  *r = res & 0xff;
}

void decrement_8bit_val(State8080 *state, uint8_t *r) {
  uint8_t res = (*r) - 1;
  state->cc.Z = (res == 0);
  state->cc.S = ((res & 0x80) != 0);
  state->cc.P = parity_8bit(res);

  (*r) = res;
}

void decrement_8bit_register_pair(uint8_t *r1, uint8_t *r2) {
  uint16_t val = (*r1) << 8 | (*r2);
  val -= 1;

  (*r1) = val >> 8;
  (*r2) = val & 0xff;
}

void add_to_A(State8080 *state, uint8_t val) {
  uint16_t res = state->A + val;

  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) == 0x80);
  state->cc.P = parity_8bit(res & 0xff);
  state->cc.CY = ((res & 0x100) == 0x100);

  state->A = res & 0xff;
}

void add_to_A_with_carry(State8080 *state, uint8_t val) {
  uint16_t res = state->A + val + (state->cc.CY ? 1 : 0);

  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) == 0x80);
  state->cc.P = parity_8bit(res & 0xff);
  state->cc.CY = ((res & 0x100) == 0x100);

  state->A = res & 0xff;
}

void subtract_from_A(State8080 *state, uint8_t val) {
  uint16_t res = state->A - val;

  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) == 0x80);
  state->cc.P = parity_8bit(res & 0xff);
  state->cc.CY = ((res & 0x100) == 0x100);

  state->A = res & 0xff;
}

void subtract_from_A_with_carry(State8080 *state, uint8_t val) {
  uint16_t res = state->A - val - (state->cc.CY ? 1 : 0);

  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) == 0x80);
  state->cc.P = parity_8bit(res & 0xff);
  state->cc.CY = ((res & 0x100) == 0x100);

  state->A = res & 0xff;
}

// assuming it's true that CY is set to 0
void bitwise_and_with_A(State8080 *state, uint8_t val) {
  uint8_t res = val & state->A;

  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) == 0x80);
  state->cc.P = parity_8bit(res & 0xff);
  state->cc.CY = 0;

  state->A = res;
}

// assuming it's true that CY is set to 0
void xor_with_A(State8080 *state, uint8_t val) {
  uint8_t res = val ^ state->A;

  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) == 0x80);
  state->cc.P = parity_8bit(res & 0xff);
  state->cc.CY = 0;

  state->A = res;
}

// assuming it's true that CY is set to 0
void bitwise_or_with_A(State8080 *state, uint8_t val) {
  uint8_t res = val | state->A;

  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) == 0x80);
  state->cc.P = parity_8bit(res & 0xff);
  state->cc.CY = 0;

  state->A = res;
}

void compare_A_with_8bit_val(State8080 *state, uint8_t val) {
  uint8_t x = state->A - val;
  state->cc.Z = (x == 0);
  state->cc.S = (0x80 == (x & 0x80));
  state->cc.P = parity_8bit(x);
  state->cc.CY = (state->A < val);
}

void add_16bit_val_to_HL(State8080 *state, uint16_t val) {
  uint32_t hl_val = state->H << 8 | state->L;
  uint32_t res = val + hl_val;

  state->cc.CY = (res & 0x10000);
  state->H = (res >> 8) & 0xff;
  state->L = res & 0xff;
}

void move_immediate_val_to_8bit_register(State8080 *state, const unsigned char *opcode, uint8_t *r) {
  (*r) = opcode[1];
  state->PC += 1;
}

int emulate_8080(State8080 *state)
{
  unsigned char *opcode = &state->memory[state->PC];
  disassemble_8080_op(state->memory, state->PC);

  // TODO: add support for AC flag
  switch (*opcode) {
    case 0x00: {		        // NOP
      break;
    }
    case 0x01: {		        // LXI    B
      direct_load_register_pair(state, opcode, &state->B, &state->C);
      break;
    }
    case 0x02: {		        // STAX   B
      store_A_indirect(state, &state->B, &state->C);
      break;
    }
    case 0x03: {		        // INX    B
      increment_8bit_register_pair(&state->B, &state->C);
      break;
    }
    case 0x04: {		        // INR    B
      increment_8bit_val(state, &state->B);
      break;
    }
    case 0x05: {        // DCR    B
      decrement_8bit_val(state, &state->B);
      break;
    }
    case 0x06: {        // MVI    B
      move_immediate_val_to_8bit_register(state, opcode, &state->B);
      break;
    }
    case 0x07: {        // RLC  (rotate A left)
      state->cc.CY = ((state->A & 0x80) == 0x80);
      uint8_t val = state->A;   
      state->A = ((val & 0x80) >> 7) | ((val << 1) & 0xff);
      break;
    }
    case 0x08: {        // NOP
      break;
    }
    case 0x09: {        // DAD    B
      uint16_t val = (state->B << 8) | state->C;
      add_16bit_val_to_HL(state, val);
      break;
    }
    case 0x0a: {        // LDAX   B
      uint16_t offset = (state->B << 8) | state->C;
      load_A_indirect(state, offset);
      break;
    }
    case 0x0b: {        // DCX    B
      decrement_8bit_register_pair(&state->B, &state->C);
      break;
    }
    case 0x0c: {       // INR    C
      increment_8bit_val(state, &state->C);
      break;
    }
    case 0x0d: {       // DCR    C
      decrement_8bit_val(state, &state->C);
      break;
    }
    case 0x0e: {       // MVI    C
      move_immediate_val_to_8bit_register(state, opcode, &state->C);
      break;
    }
    case 0x0f: {       // RRC
      state->cc.CY = (state->A & 1 == 1);
      uint8_t val = state->A;   
      state->A = ((val & 1) << 7) | (val >> 1);
      break;
    }
    case 0x10: {       // NOP
      break;
    }
    case 0x11: {       // LXI    D
      direct_load_register_pair(state, opcode, &state->D, &state->E);
      break;
    }
    case 0x12: {       // STAX   D
      store_A_indirect(state, &state->D, &state->E);
      break;
    }
    case 0x13: {       // INX    D
      increment_8bit_register_pair(&state->D, &state->E);
      break;
    }
    case 0x14: {       // INR    D
      increment_8bit_val(state, &state->D);
      break;
    }
    case 0x15: {       // DCR    D
      decrement_8bit_val(state, &state->D);
      break;
    }
    case 0x16: {       // MVI    D
      move_immediate_val_to_8bit_register(state, opcode, &state->D);
      break;
    }
    case 0x17: {       // RAL
      uint8_t val = state->A;   
      state->A = state->cc.CY | ((val << 1) & 0xff);
      state->cc.CY = ((state->A & 0x80) == 0x80);
      break;
    }
    case 0x18: {       // NOP
      break;
    }
    case 0x19: {       // DAD    D
      uint16_t val = (state->D << 8) | state->E;
      add_16bit_val_to_HL(state, val);
      break;
    }
    case 0x1a: {       // LDAX   D
      uint16_t offset = (state->D << 8) | state->E;
      load_A_indirect(state, offset);
      break;
    }
    case 0x1b: {       // DCX    D
      decrement_8bit_register_pair(&state->D, &state->E);
      break;
    }
    case 0x1c: {       // INR    E
      increment_8bit_val(state, &state->E);
      break;
    }
    case 0x1d: {       // DCR    E
      decrement_8bit_val(state, &state->E);
      break;
    }
    case 0x1e: {       // MVI    E
      move_immediate_val_to_8bit_register(state, opcode, &state->E);
      break;
    }
    case 0x1f: {       // RAR
      state->cc.CY = (state->A & 1 == 1);
      uint8_t val = state->A;
      state->A = (state->cc.CY << 7) | (val >> 1);
      break;
    }
    case 0x20: {       // NOP
      break;
    }
    case 0x21: {       // LXI    H
      direct_load_register_pair(state, opcode, &state->H, &state->L);
      break;
    }
    case 0x22: {       // SHLD
      uint16_t addr = (opcode[2] << 8) | opcode[1];
      state->memory[addr] = state->L;
      state->memory[addr + 1] = state->H; 
      
      state->PC += 2;
      break;
    }
    case 0x23: {       // INX    H
      increment_8bit_register_pair(&state->H, &state->L);
      break;
    }
    case 0x24: {       // INR    H
      increment_8bit_val(state, &state->H);
      break;
    }
    case 0x25: {       // DCR    H
      decrement_8bit_val(state, &state->H);
      break;
    }
    case 0x26: {       // MVI    H
      move_immediate_val_to_8bit_register(state, opcode, &state->H);
      break;
    }
    case 0x27: {       // DAA
      printf("Not implemented instruction (DAA) called");
      break;
    }
    case 0x28: {       // NOP
      break;
    }
    case 0x29: {       // DAD    H
      uint16_t val = (state->H << 8) | state->L;
      add_16bit_val_to_HL(state, val);
      break;
    }
    case 0x2a: {       // LHLD
      uint16_t addr = (opcode[2] << 8) | opcode[1];
      state->L = state->memory[addr];
      state->H = state->memory[addr + 1];

      state->PC += 2;
      break;
    }
    case 0x2b: {       // DCX    H
      decrement_8bit_register_pair(&state->H, &state->L);
      break;
    }
    case 0x2c: {       // INR    L
      increment_8bit_val(state, &state->L);
      break;
    }
    case 0x2d: {       // DCR    L
      decrement_8bit_val(state, &state->L);
      break;
    }
    case 0x2e: {       // MVI    L
      move_immediate_val_to_8bit_register(state, opcode, &state->L);
      break;
    }
    case 0x2f: {       // CMA
      state->A = ~state->A;
      break;
    }
    case 0x30: {       // NOP
      break;
    }
    case 0x31: {       // LXI    SP
      uint16_t val = (opcode[2] << 8) | opcode[1];
      state->SP = val;
      state->PC += 2;
      break;
    }
    case 0x32: {       // STA
      uint16_t offset = (opcode[2] << 8) | opcode[1];
      state->memory[offset] = state->A;
      state->PC += 2;
      break;
    }
    case 0x33: {       // INX    SP
      state->SP++;
      break;
    }
    case 0x34: {       // INR    M
      uint16_t offset = (state->H << 8) | state->L;
      increment_8bit_val(state, &state->memory[offset]);
      break;
    }
    case 0x35: {       // DCR    M
      uint16_t offset = (state->H << 8) | state->L;
      decrement_8bit_val(state, &state->memory[offset]);
      break;
    }
    case 0x36: {       // MVI    M
      uint16_t offset = (state->H << 8) | state->L;
      state->memory[offset] = opcode[1];
      state->PC++;
      break;
    }
    case 0x37: {       // STC
      state->cc.CY = 1;
      break;
    }
    case 0x38: {       // NOP
      break;
    }
    case 0x39: {       // DAD    SP
      add_16bit_val_to_HL(state, state->SP);
      break;
    }
    case 0x3a: {       // LDA
      uint16_t offset = (opcode[2] << 8) | opcode[1];
      load_A_indirect(state, offset);
      state->PC += 2;
      break;
    }
    case 0x3b: {       // DCX    SP
      state->SP--;
      break;
    }
    case 0x3c: {       // INR    A
      increment_8bit_val(state, &state->A);
      break;
    }
    case 0x3d: {       // DCR    A
      decrement_8bit_val(state, &state->A);
      break;
    }
    case 0x3e: {       // MVI    A
      move_immediate_val_to_8bit_register(state, opcode, &state->A);
      break;
    }
    case 0x3f: {       // CMC
      state->cc.CY = ~state->cc.CY;
      break;
    }
    case 0x40: {       // MOV    B,B
      move_8bit(&state->B, state->B);
      break;
    }
    case 0x41: {       // MOV    B,C
      move_8bit(&state->B, state->C);
      break;
    }
    case 0x42: {       // MOV    B,D
      move_8bit(&state->B, state->D);
      break;
    }
    case 0x43: {       // MOV    B,E
      move_8bit(&state->B, state->E);
      break;
    }
    case 0x44: {       // MOV    B,H
      move_8bit(&state->B, state->H);
      break;
    }
    case 0x45: {       // MOV    B,L
      move_8bit(&state->B, state->L);
      break;
    }
    case 0x46: {       // MOV    B,M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      move_8bit(&state->B, val);
      break;
    }
    case 0x47: {       // MOV    B,A
      move_8bit(&state->B, state->A);
      break;
    }
    case 0x48: {       // MOV    C,B
      move_8bit(&state->C, state->B);
      break;
    }
    case 0x49: {       // MOV    C,C
      move_8bit(&state->C, state->C);
      break;
    }
    case 0x4a: {       // MOV    C,D
      move_8bit(&state->C, state->D);
      break;
    }
    case 0x4b: {       // MOV    C,E
      move_8bit(&state->C, state->E);
      break;
    }
    case 0x4c: {       // MOV    C,H
      move_8bit(&state->C, state->H);
      break;
    }
    case 0x4d: {       // MOV    C,L
      move_8bit(&state->C, state->L);
      break;
    }
    case 0x4e: {       // MOV    C,M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      move_8bit(&state->C, val);
      break;
    }
    case 0x4f: {       // MOV    C,A
      move_8bit(&state->C, state->A);
      break;
    }
    case 0x50: {       // MOV    D,B
      move_8bit(&state->D, state->B);
      break;
    }
    case 0x51: {       // MOV    D,C
      move_8bit(&state->D, state->C);
      break;
    }
    case 0x52: {       // MOV    D,D
      move_8bit(&state->D, state->D);
      break;
    }
    case 0x53: {       // MOV    D,E
      move_8bit(&state->D, state->E);
      break;
    }
    case 0x54: {       // MOV    D,H
      move_8bit(&state->D, state->H);
      break;
    }
    case 0x55: {       // MOV    D,L
      move_8bit(&state->D, state->L);
      break;
    }
    case 0x56: {       // MOV    D,M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      move_8bit(&state->D, val);
      break;
    }
    case 0x57: {       // MOV    D,A
      move_8bit(&state->D, state->A);
      break;
    }
    case 0x58: {       // MOV    E,B
      move_8bit(&state->E, state->B);
      break;
    }
    case 0x59: {       // MOV    E,C
      move_8bit(&state->E, state->C);
      break;
    }
    case 0x5a: {       // MOV    E,D
      move_8bit(&state->E, state->D);
      break;
    }
    case 0x5b: {       // MOV    E,E
      move_8bit(&state->E, state->E);
      break;
    }
    case 0x5c: {       // MOV    E,H
      move_8bit(&state->E, state->H);
      break;
    }
    case 0x5d: {       // MOV    E,L
      move_8bit(&state->E, state->L);
      break;
    }
    case 0x5e: {       // MOV    E,M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      move_8bit(&state->E, val);
      break;
    }
    case 0x5f: {       // MOV    E,A
      move_8bit(&state->E, state->A);
      break;
    }
    case 0x60: {       // MOV    H,B
      move_8bit(&state->H, state->B);
      break;
    }
    case 0x61: {       // MOV    H,C
      move_8bit(&state->H, state->C);
      break;
    }
    case 0x62: {       // MOV    H,D
      move_8bit(&state->H, state->D);
      break;
    }
    case 0x63: {       // MOV    H,E
      move_8bit(&state->H, state->E);
      break;
    }
    case 0x64: {       // MOV    H,H
      move_8bit(&state->H, state->H);
      break;
    }
    case 0x65: {       // MOV    H,L
      move_8bit(&state->H, state->L);
      break;
    }
    case 0x66: {       // MOV    H,M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      move_8bit(&state->H, val);
      break;
    }
    case 0x67: {       // MOV    H,A
      move_8bit(&state->H, state->A);
      break;
    }
    case 0x68: {       // MOV    L,B
      move_8bit(&state->L, state->B);
      break;
    }
    case 0x69: {       // MOV    L,C
      move_8bit(&state->L, state->C);
      break;
    }
    case 0x6a: {       // MOV    L,D
      move_8bit(&state->L, state->D);
      break;
    }
    case 0x6b: {       // MOV    L,E
      move_8bit(&state->L, state->E);
      break;
    }
    case 0x6c: {       // MOV    L,H
      move_8bit(&state->L, state->H);
      break;
    }
    case 0x6d: {       // MOV    L,L
      move_8bit(&state->L, state->L);
      break;
    }
    // TODO: rework to avoid copy-paste
    case 0x6e: {       // MOV    L,M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      move_8bit(&state->L, val);
      break;
    }
    case 0x6f: {       // MOV    L,A
      move_8bit(&state->L, state->A);
      break;
    }
    case 0x70: {       // MOV    M,B
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t m_val = state->memory[offset];
      move_8bit(&m_val, state->B);
      break;
    }
    case 0x71: {       // MOV    M,C
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t m_val = state->memory[offset];
      move_8bit(&m_val, state->C);
      break;
    }
    case 0x72: {       // MOV    M,D
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t m_val = state->memory[offset];
      move_8bit(&m_val, state->D);
      break;
    }
    case 0x73: {       // MOV    M,E
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t m_val = state->memory[offset];
      move_8bit(&m_val, state->E);
      break;
    }
    case 0x74: {       // MOV    M,H
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t m_val = state->memory[offset];
      move_8bit(&m_val, state->H);
      break;
    }
    case 0x75: {       // MOV    M,L
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t m_val = state->memory[offset];
      move_8bit(&m_val, state->L);
      break;
    }
    case 0x76: {       // HLT
      exit(0);
      break;
    }
    case 0x77: {       // MOV    M,A
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t m_val = state->memory[offset];
      move_8bit(&m_val, state->A);
      break;
    }
    case 0x78: {       // MOV    A,B
      move_8bit(&state->A, state->B);
      break;
    }
    case 0x79: {       // MOV    A,C
      move_8bit(&state->A, state->C);
      break;
    }
    case 0x7a: {       // MOV    A,D
      move_8bit(&state->A, state->D);
      break;
    }
    case 0x7b: {       // MOV    A,E
      move_8bit(&state->A, state->E);
      break;
    }
    case 0x7c: {       // MOV    A,H
      move_8bit(&state->A, state->H);
      break;
    }
    case 0x7d: {       // MOV    A,L
      move_8bit(&state->A, state->L);
      break;
    }
    case 0x7e: {       // MOV    A,M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      move_8bit(&state->A, val);
      break;
    }
    case 0x7f: {       // MOV    A,A
      move_8bit(&state->A, state->A);
      break;
    }
    case 0x80: {       // ADD    B
      add_to_A(state, state->B);
      break;
    }
    case 0x81: {       // ADD    C
      add_to_A(state, state->C);
      break;
    }
    case 0x82: {       // ADD    D
      add_to_A(state, state->D);
      break;
    }
    case 0x83: {       // ADD    E
      add_to_A(state, state->E);
      break;
    }
    case 0x84: {       // ADD    H
      add_to_A(state, state->H);
      break;
    }
    case 0x85: {       // ADD    L
      add_to_A(state, state->L);
      break;
    }
    case 0x86: {       // ADD    M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      add_to_A(state, val);
      break;
    }
    case 0x87: {       // ADD    A
      add_to_A(state, state->A);
      break;
    }
    case 0x88: {       // ADC    B
      add_to_A_with_carry(state, state->B);
      break;
    }
    case 0x89: {       // ADC    C
      add_to_A_with_carry(state, state->C);
      break;
    }
    case 0x8a: {       // ADC    D
      add_to_A_with_carry(state, state->D);
      break;
    }
    case 0x8b: {       // ADC    E
      add_to_A_with_carry(state, state->E);
      break;
    }
    case 0x8c: {       // ADC    H
      add_to_A_with_carry(state, state->H);
      break;
    }
    case 0x8d: {       // ADC    L
      add_to_A_with_carry(state, state->L);
      break;
    }
    case 0x8e: {       // ADC    M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      add_to_A_with_carry(state, val);
      break;
    }
    case 0x8f: {       // ADC    A
      add_to_A_with_carry(state, state->A);
      break;
    }
    case 0x90: {       // SUB    B
      subtract_from_A(state, state->B);
      break;
    }
    case 0x91: {       // SUB    C
      subtract_from_A(state, state->C);
      break;
    }
    case 0x92: {       // SUB    D
      subtract_from_A(state, state->D);
      break;
    }
    case 0x93: {       // SUB    E
      subtract_from_A(state, state->E);
      break;
    }
    case 0x94: {       // SUB    H
      subtract_from_A(state, state->H);
      break;
    }
    case 0x95: {       // SUB    L
      subtract_from_A(state, state->L);
      break;
    }
    case 0x96: {       // SUB    M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      subtract_from_A(state, val);
      break;
    }
    case 0x97: {       // SUB    A
      subtract_from_A(state, state->A);
      break;
    }
    case 0x98: {       // SBB    B
      subtract_from_A_with_carry(state, state->B);
      break;
    }
    case 0x99: {       // SBB    C
      subtract_from_A_with_carry(state, state->C);
      break;
    }
    case 0x9a: {       // SBB    D
      subtract_from_A_with_carry(state, state->D);
      break;
    }
    case 0x9b: {       // SBB    E
      subtract_from_A_with_carry(state, state->E);
      break;
    }
    case 0x9c: {       // SBB    H
      subtract_from_A_with_carry(state, state->H);
      break;
    }
    case 0x9d: {       // SBB    L
      subtract_from_A_with_carry(state, state->L);
      break;
    }
    case 0x9e: {       // SBB    M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      subtract_from_A_with_carry(state, val);
      break;
    }
    case 0x9f: {       // SBB    A
      subtract_from_A_with_carry(state, state->A);
      break;
    }
    case 0xa0: {       // ANA    B
      bitwise_and_with_A(state, state->B);
      break;
    }
    case 0xa1: {       // ANA    C
      bitwise_and_with_A(state, state->C);
      break;
    }
    case 0xa2: {       // ANA    D
      bitwise_and_with_A(state, state->D);
      break;
    }
    case 0xa3: {       // ANA    E
      bitwise_and_with_A(state, state->E);
      break;
    }
    case 0xa4: {       // ANA    H
      bitwise_and_with_A(state, state->H);
      break;
    }
    case 0xa5: {       // ANA    L
      bitwise_and_with_A(state, state->L);
      break;
    }
    case 0xa6: {       // ANA    M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      bitwise_and_with_A(state, val);
      break;
    }
    case 0xa7: {       // ANA    A
      bitwise_and_with_A(state, state->A);
      break;
    }
    case 0xa8: {       // XRA    B
      xor_with_A(state, state->B);
      break;
    }
    case 0xa9: {       // XRA    C
      xor_with_A(state, state->C);
      break;
    }
    case 0xaa: {       // XRA    D
      xor_with_A(state, state->D);
      break;
    }
    case 0xab: {       // XRA    E
      xor_with_A(state, state->E);
      break;
    }
    case 0xac: {       // XRA    H
      xor_with_A(state, state->H);
      break;
    }
    case 0xad: {       // XRA    L
      xor_with_A(state, state->L);
      break;
    }
    case 0xae: {       // XRA    M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      xor_with_A(state, val);
      break;
    }
    case 0xaf: {       // XRA    A
      xor_with_A(state, state->A);
      break;
    }
    case 0xb0: {       // ORA    B
      bitwise_or_with_A(state, state->B);
      break;
    }
    case 0xb1: {       // ORA    C
      bitwise_or_with_A(state, state->C);
      break;
    }
    case 0xb2: {       // ORA    D
      bitwise_or_with_A(state, state->D);
      break;
    }
    case 0xb3: {       // ORA    E
      bitwise_or_with_A(state, state->E);
      break;
    }
    case 0xb4: {       // ORA    H
      bitwise_or_with_A(state, state->H);
      break;
    }
    case 0xb5: {       // ORA    L
      bitwise_or_with_A(state, state->L);
      break;
    }
    case 0xb6: {       // ORA    M
      uint16_t offset = (state->H << 8) | state->L;
      uint8_t val = state->memory[offset];
      bitwise_or_with_A(state, state->B);
      break;
    }
    case 0xb7: {       // ORA    A
      bitwise_or_with_A(state, state->A);
      break;
    }
    case 0xb8: {       // CMP    B
      compare_A_with_8bit_val(state, state->B);
      break;
    }
    case 0xb9: {       // CMP    C
      compare_A_with_8bit_val(state, state->C);
      break;
    }
    case 0xba: {       // CMP    D
      compare_A_with_8bit_val(state, state->D);
      break;
    }
    case 0xbb: {       // CMP    E
      compare_A_with_8bit_val(state, state->E);
      break;
    }
    case 0xbc: {       // CMP    H
      compare_A_with_8bit_val(state, state->H);
      break;
    }
    case 0xbd: {       // CMP    L
      compare_A_with_8bit_val(state, state->L);
      break;
    }
    case 0xbe: {       // CMP    M
      uint16_t val_addr = (state->H << 8) | state->L;
      uint8_t val = state->memory[val_addr];
      compare_A_with_8bit_val(state, val);
      break;
    }
    case 0xbf: {       // CMP    A
      compare_A_with_8bit_val(state, state->A);
      break;
    }
    case 0xc0: {       // RNZ
      if (state->cc.Z == 0) call_return(state);
      break;
    }
    case 0xc1: {       // POP    B
      pop_stack(state, &state->B, &state->C);
      break;
    }
    case 0xc2: {       // JNZ
      if (state->cc.Z == 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xc3: {       // JMP
      state->PC = (opcode[2] << 8) | opcode[1];
      break;
    }
    case 0xc4: {       // CNZ
      if (state->cc.Z == 0) {
        const uint16_t addr = (opcode[2] << 8) | opcode[1];
        call_addr(state, addr);
      }
      else state->PC += 2;
      break;
    }
    case 0xc5: {       // PUSH   B
      push_stack(state, state->B, state->C);
      break;
    }
    case 0xc6: {       // ADI
      add_to_A(state, opcode[1]);
      state->PC++;
      break;
    }
    case 0xc7: {       // RST    0
      call_addr(state, 0x00);
      break;
    }
    case 0xc8: {       // RZ
      if (state->cc.Z != 0) call_return(state);
      break;
    }
    case 0xc9: {       // RET
      call_return(state);
      break;
    }
    case 0xca: {       // JZ
      if (state->cc.Z != 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xcb: {       // NOP
      break;
    }
    case 0xcc: {       // CZ
      if (state->cc.Z != 0) {
        const uint16_t addr = (opcode[2] << 8) | opcode[1];
        call_addr(state, addr);
      }
      else state->PC += 2;
      break;
    }
    case 0xcd: {       // CALL
      const uint16_t addr = (opcode[2] << 8) | opcode[1];
      call_addr(state, addr);
      break;
    }
    case 0xce: {       // ACI
      add_to_A_with_carry(state, opcode[1]);
      state->PC++;
      break;
    }
    case 0xcf: {       // RST   1
      call_addr(state, 0x08);
      break;
    }
    case 0xd0: {       // RNC
      if (state->cc.CY == 0) call_return(state);
      break;
    }
    case 0xd1: {       // POP    D
      pop_stack(state, &state->D, &state->E);
      break;
    }
    case 0xd2: {       // JNC
      if (state->cc.CY == 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xd3: {       // OUT
      uint8_t out = opcode[1];
      state->PC++;
      break;
    }
    case 0xd4: {       // CNC
      if (state->cc.CY == 0) {
        const uint16_t addr = (opcode[2] << 8) | opcode[1];
        call_addr(state, addr);
      }
      else state->PC += 2;
      break;
    }
    case 0xd5: {       // PUSH   D
      push_stack(state, state->D, state->E);
      break;
    }
    case 0xd6: {       // SUI
      subtract_from_A(state, opcode[1]);
      state->PC++;
      break;
    }
    case 0xd7: {       // RST   2
      call_addr(state, 0x10);
      break;
    }
    case 0xd8: {       // RC
      if (state->cc.CY != 0) call_return(state);
      break;
    }
    case 0xd9: {       // NOP
      break;
    }
    case 0xda: {       // JC
      if (state->cc.CY != 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xdb: {       // IN
      // uint8_t inp = opcode[1];
      state->PC++;
      break;
    }
    case 0xdc: {       // CC
      if (state->cc.CY == 1) {
        const uint16_t addr = (opcode[2] << 8) | opcode[1];
        call_addr(state, addr);
      }
      else state->PC += 2;
      break;
    }
    case 0xdd: {       // NOP
      break;
    }
    case 0xde: {       // SBI
      subtract_from_A_with_carry(state, opcode[1]);
      state->PC++;
      break;
    }
    case 0xdf: {       // RST   3
      call_addr(state, 0x18);
      break;
    }
    case 0xe0: {       // RPO
      if (state->cc.P == 0) call_return(state);
      break;
    }
    case 0xe1: {       // POP    H
      pop_stack(state, &state->H, &state->L);
      break;
    }
    case 0xe2: {       // JPO
      if (state->cc.P == 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xe3: {       // XTHL
      uint8_t tmp = state->H;
      state->H = state->memory[state->SP + 1];
      state->memory[state->SP + 1] = tmp;

      tmp = state->L;
      state->L = state->memory[state->SP];
      state->memory[state->SP] = tmp;
      break;
    }
    case 0xe4: {       // CPO
      if (state->cc.P == 0) {
        const uint16_t addr = (opcode[2] << 8) | opcode[1];
        call_addr(state, addr);
      }
      else state->PC += 2;
      break;
    }
    case 0xe5: {       // PUSH   H
      push_stack(state, state->H, state->L);
      break;
    }
    case 0xe6: {       // ANI
      bitwise_and_with_A(state, opcode[1]);
      state->PC++;
      break;
    }
    case 0xe7: {       // RST   4
      call_addr(state, 0x20);
      break;
    }
    case 0xe8: {       // RPE
      if (state->cc.CY != 0) call_return(state);
      break;
    }
    case 0xe9: {       // PCHL
      uint16_t val = (state->H << 8) | state->L;
      state->PC = val;
      break;
    }
    case 0xea: {       // JPE
      if (state->cc.P != 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xeb: {       // XCHG
      uint8_t tmp = state->H;
      state->H = state->D;
      state->D = tmp;

      tmp = state->L;
      state->L = state->E;
      state->E = tmp;
      break;
    }
    case 0xec: {       // CPE
      if (state->cc.Z != 0) {
        const uint16_t addr = (opcode[2] << 8) | opcode[1];
        call_addr(state, addr);
      }
      else state->PC += 2;
      break;
    }
    case 0xed: {       // NOP
      break;
    }
    case 0xee: {       // XRI
      xor_with_A(state, opcode[1]);
      state->PC++;
      break;
    }
    case 0xef: {       // RST     5
      call_addr(state, 0x28);
      break;
    }
    case 0xf0: {       // RP
      if (state->cc.S == 0) call_return(state);
      break;
    }
    case 0xf1: {       // POP    PSW
      state->A = state->memory[state->SP + 1];

      uint8_t psw = state->memory[state->SP];
      state->cc.Z = (psw & 0x01 == 0x01);
      state->cc.S = (psw & 0x02 == 0x02);
      state->cc.P = (psw & 0x04 == 0x04);
      state->cc.CY = (psw & 0x08 == 0x08);
      state->cc.AC = (psw & 0x10 == 0x10);

      state->SP += 2;
      break;
    }
    case 0xf2: {       // JP
      if (state->cc.S == 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xf3: {       // DI
      state->int_enable = 0;
      break;
    }
    case 0xf4: {       // CP
      if (state->cc.S == 0) {
        const uint16_t addr = (opcode[2] << 8) | opcode[1];
        call_addr(state, addr);
      }
      else state->PC += 2;
      break;
    }
    case 0xf5: {       // PUSH   PSW
      uint8_t flags_byte = 0;
      flags_byte |= state->cc.Z;
      flags_byte |= (state->cc.S << 1);
      flags_byte |= (state->cc.P << 2);
      flags_byte |= (state->cc.CY << 3);
      flags_byte |= (state->cc.AC << 4);

      push_stack(state, state->A, flags_byte);
      break;
    }
    case 0xf6: {       // ORI
      bitwise_or_with_A(state, opcode[1]);
      state->PC++;
      break;
    }
    case 0xf7: {       // RST    6
      call_addr(state, 0x30);
      break;
    }
    case 0xf8: {       // RM
      if (state->cc.CY != 0) call_return(state);
      break;
    }
    case 0xf9: {       // SPHL
      uint16_t val = (state->H << 8) | state->L;
      state->SP = val;
      break;
    }
    case 0xfa: {       // JM
      if (state->cc.S != 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xfb: {       // EI
      state->int_enable = 1;
      break;
    }
    case 0xfc: {       // CM
      if (state->cc.S != 0) {
        const uint16_t addr = (opcode[2] << 8) | opcode[1];
        call_addr(state, addr);
      }
      else state->PC += 2;
      break;
    }
    case 0xfd: {       // NOP
      break;
    }
    case 0xfe: {       // CPI
      uint8_t val = opcode[1];
      compare_A_with_8bit_val(state, val);
      state->PC++;
      break;
    }
    case 0xff: {       // RST     7
      call_addr(state, 0x38);
      break;
    }

    default:
      printf("Error: hex '%02x' not recognized!", *opcode);
      return 1;
  }

  
  printf("\tZ=%d, S=%d, P=%d, CY=%d\n", state->cc.Z, state->cc.S, state->cc.P, state->cc.CY);    
  printf("\tA=$%02x B=$%02x C=$%02x D=$%02x E=$%02x H=$%02x L=$%02x SP=%04x\n",    
    state->A, state->B, state->C, state->D,    
    state->E, state->H, state->L, state->SP); 
  return 0;
}

int invoke_emulation(FILE *filepath) {
  State8080 state = {NULL};
  emulate_8080(&state);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("Please specify the file path as the input argument!\n");
    return 1;
  }

  invoke_emulation(argv[1]);

  return 0;
}