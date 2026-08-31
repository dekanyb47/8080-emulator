#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BIT_MASK(bit) (1 << (bit))
#define SET_BIT(value,bit) ((value) |= BIT_MASK(bit))
#define CLEAR_BIT(value,bit) ((value) &= ~BIT_MASK(bit))
#define TEST_BIT(value,bit) (((value) & BIT_MASK(bit)) ? 1 : 0)

typedef struct ConditionCodes
{
  uint8_t S : 1;
  uint8_t Z : 1;
  uint8_t AC : 1;
  uint8_t P : 1;
  uint8_t CY : 1;
  uint8_t pad : 3;
} ConditionCodes;

typedef struct State8080
{
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

void unimplentedInstruction()
{
  printf("Unimplemented instruction!\n");
  exit(1);
}

int parity(uint8_t r) {
  return 1;
}


// TODO: finish
void call_addr(State8080 *state, const uint16_t addr) {
  uint16_t ret_addr = state->PC+2;
  state->memory[state->SP - 1] = (ret_addr >> 8) & 0xff;
}

void direct_load_register_pair(State8080 *state, const unsigned char *opcode, uint8_t *r1, uint8_t *r2) {
  *r1 = opcode[1];
  *r2 = opcode[2];
  state->PC += 2;
}

void store_A_indirect(State8080 *state, const uint8_t *r1, const uint8_t *r2) {
  const uint16_t offset = ((*r1) << 8) | (*r2);
  state->memory[offset] = state->A;
}

void load_A_indirect(State8080 *state, const uint8_t *r1, const uint8_t *r2) {
  uint16_t offset = (*r1) << 8 | (*r2);
  uint8_t val = state->memory[offset];
  state->A = val;
}

void increment_8bit_register_pair(uint8_t *r1, uint8_t *r2) {
  (*r2) += 1;
  if ((*r2) == 0)
    (*r1)++;
}

void increment_8bit_register(State8080 *state, uint8_t *r) {
  uint16_t res = (uint16_t)(*r) + 1;
  state->cc.Z = ((res & 0xff) == 0);
  state->cc.S = ((res & 0x80) != 0);
  state->cc.P = parity(res & 0xff);

  *r = res & 0xff;
}

void decrement_8bit_register(State8080 *state, uint8_t *r) {
  uint8_t res = (*r) - 1;
  state->cc.Z = (res == 0);
  state->cc.S = ((res & 0x80) != 0);
  state->cc.P = parity(res);

  (*r) = res;
}

void decrement_8bit_register_pair(uint8_t *r1, uint8_t *r2) {
  uint16_t val = (*r1) << 8 | (*r2);
  val -= 1;

  (*r1) = val >> 8;
  (*r2) = val & 0xff;
}

void add_register_pair_to_HL(State8080 *state, const uint8_t *r1, const uint8_t *r2) {
  uint32_t rpair_val = (*r1) << 8 | (*r2);
  uint32_t hl_val = state->H << 8 | state->L;
  uint32_t res = rpair_val + hl_val;

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
      increment_8bit_register(state, &state->B);
      break;
    }
    case 0x05: {        // DCR    B
      decrement_8bit_register(state, &state->B);
      break;
    }
    case 0x06: {        // MVI    B
      move_immediate_val_to_8bit_register(state, opcode, &state->B);
      break;
    }
    case 0x07: {        // RLC  (rotate A left)
      uint16_t res = state->A << 1;
      state->cc.CY = (TEST_BIT(res, 8) == 1);
      if (res & 0x100) SET_BIT(res, 0);
      state->A = res & 0xff;
      break;
    }
    case 0x08: {        // NOP
      break;
    }
    case 0x09: {        // DAD    B
      add_register_pair_to_HL(state, &state->B, &state->C);
      break;
    }
    case 0x0a: {        // LDAX   B
      load_A_indirect(state, &state->B, &state->C);
      break;
    }
    case 0x0b: {        // DCX    B
      decrement_8bit_register_pair(&state->B, &state->C);
      break;
    }
    case 0x0c: {       // INR    C
      increment_8bit_register(state, &state->C);
      break;
    }
    case 0x0d: {       // DCR    C
      decrement_8bit_register(state, &state->C);
      break;
    }
    case 0x0e: {       // MVI    C
      move_immediate_val_to_8bit_register(state, opcode, &state->C);
      break;
    }
    case 0x0f: {       // RRC
      uint16_t res = state->A;
      if (TEST_BIT(res, 0)) SET_BIT(res, 8);
      res = res >> 1;
      state->A = res;
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
      increment_8bit_register(state, &state->D);
      break;
    }
    case 0x15: {       // DCR    D
      decrement_8bit_register(state, &state->D);
      break;
    }
    case 0x16: {       // MVI    D
      move_immediate_val_to_8bit_register(state, opcode, &state->D);
      break;
    }
    case 0x17: {       // RAL
      break;
    }
    case 0x18: {       // NOP
      break;
    }
    case 0x19: {       // DAD    D
      add_register_pair_to_HL(state, &state->D, &state->E);
      break;
    }
    case 0x1a: {       // LDAX   D
      load_A_indirect(state, &state->D, &state->E);
      break;
    }
    case 0x1b: {       // DCX    D
      decrement_8bit_register_pair(&state->D, &state->E);
      break;
    }
    case 0x1c: {       // INR    E
      increment_8bit_register(state, &state->E);
      break;
    }
    case 0x1d: {       // DCR    E
      decrement_8bit_register(state, &state->E);
      break;
    }
    case 0x1e: {       // MVI    E
      move_immediate_val_to_8bit_register(state, opcode, &state->E);
      break;
    }
    case 0x1f: {       // RAR
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
      break;
    }
    case 0x23: {       // INX    H
      increment_8bit_register_pair(&state->H, &state->L);
      break;
    }
    case 0x24: {       // INR    H
      increment_8bit_register(state, &state->H);
      break;
    }
    case 0x25: {       // DCR    H
      decrement_8bit_register(state, &state->H);
      break;
    }
    case 0x26: {       // MVI    H
      move_immediate_val_to_8bit_register(state, opcode, &state->H);
      break;
    }
    case 0x27: {       // DAA
      break;
    }
    case 0x28: {       // NOP
      break;
    }
    case 0x29: {       // DAD    H
      add_register_pair_to_HL(state, &state->H, &state->L);
      break;
    }
    case 0x2a: {       // LHLD
      break;
    }
    case 0x2b: {       // DCX    H
      decrement_8bit_register_pair(&state->H, &state->L);
      break;
    }
    case 0x2c: {       // INR    L
      increment_8bit_register(state, &state->L);
      break;
    }
    case 0x2d: {       // DCR    L
      decrement_8bit_register(state, &state->L);
      break;
    }
    case 0x2e: {       // MVI    L
      move_immediate_val_to_8bit_register(state, opcode, &state->L);
      break;
    }
    case 0x2f: {       // CMA
      break;
    }
    case 0x30: {       // NOP
      break;
    }
    case 0x31: {       // LXI    SP

      break;
    }
    case 0x32: {       // STA
      break;
    }
    case 0x33: {       // INX    SP
      break;
    }
    case 0x34: {       // INR    M
      break;
    }
    case 0x35: {       // DCR    M
      break;
    }
    case 0x36: {       // MVI    M
      break;
    }
    case 0x37: {       // STC
      break;
    }
    case 0x38: {       // NOP
      break;
    }
    case 0x39: {       // DAD    SP
      break;
    }
    case 0x3a: {       // LDA
      break;
    }
    case 0x3b: {       // DCX    SP
      break;
    }
    case 0x3c: {       // INR    A
      increment_8bit_register(state, &state->A);
      break;
    }
    case 0x3d: {       // DCR    A
      decrement_8bit_register(state, &state->A);
      break;
    }
    case 0x3e: {       // MVI    A
      break;
    }
    case 0x3f: {       // CMC
      break;
    }
    case 0x40: {       // MOV    B,B
      break;
    }
    case 0x41: {       // MOV    B,C
      break;
    }
    case 0x42: {       // MOV    B,D
      break;
    }
    case 0x43: {       // MOV    B,E
      break;
    }
    case 0x44: {       // MOV    B,H
      break;
    }
    case 0x45: {       // MOV    B,L
      break;
    }
    case 0x46: {       // MOV    B,M
      break;
    }
    case 0x47: {       // MOV    B,A
      break;
    }
    case 0x48: {       // MOV    C,B
      break;
    }
    case 0x49: {       // MOV    C,C
      break;
    }
    case 0x4a: {       // MOV    C,D
      break;
    }
    case 0x4b: {       // MOV    C,E
      break;
    }
    case 0x4c: {       // MOV    C,H
      break;
    }
    case 0x4d: {       // MOV    C,L
      break;
    }
    case 0x4e: {       // MOV    C,M
      break;
    }
    case 0x4f: {       // MOV    C,A
      break;
    }
    case 0x50: {       // MOV    D,B
      break;
    }
    case 0x51: {       // MOV    D,C
      break;
    }
    case 0x52: {       // MOV    D,D
      break;
    }
    case 0x53: {       // MOV    D,E
      break;
    }
    case 0x54: {       // MOV    D,H
      break;
    }
    case 0x55: {       // MOV    D,L
      break;
    }
    case 0x56: {       // MOV    D,M
      break;
    }
    case 0x57: {       // MOV    D,A
      break;
    }
    case 0x58: {       // MOV    E,B
      break;
    }
    case 0x59: {       // MOV    E,C
      break;
    }
    case 0x5a: {       // MOV    E,D
      break;
    }
    case 0x5b: {       // MOV    E,E
      break;
    }
    case 0x5c: {       // MOV    E,H
      break;
    }
    case 0x5d: {       // MOV    E,L
      break;
    }
    case 0x5e: {       // MOV    E,M
      break;
    }
    case 0x5f: {       // MOV    E,A
      break;
    }
    case 0x60: {       // MOV    H,B
      break;
    }
    case 0x61: {       // MOV    H,C
      break;
    }
    case 0x62: {       // MOV    H,D
      break;
    }
    case 0x63: {       // MOV    H,E
      break;
    }
    case 0x64: {       // MOV    H,H
      break;
    }
    case 0x65: {       // MOV    H,L
      break;
    }
    case 0x66: {       // MOV    H,M
      break;
    }
    case 0x67: {       // MOV    H,A
      break;
    }
    case 0x68: {       // MOV    L,B
      break;
    }
    case 0x69: {       // MOV    L,C
      break;
    }
    case 0x6a: {       // MOV    L,D
      break;
    }
    case 0x6b: {       // MOV    L,E
      break;
    }
    case 0x6c: {       // MOV    L,H
      break;
    }
    case 0x6d: {       // MOV    L,L
      break;
    }
    case 0x6e: {       // MOV    L,M
      break;
    }
    case 0x6f: {       // MOV    L,A
      break;
    }
    case 0x70: {       // MOV    M,B
      break;
    }
    case 0x71: {       // MOV    M,C
      break;
    }
    case 0x72: {       // MOV    M,D
      break;
    }
    case 0x73: {       // MOV    M,E
      break;
    }
    case 0x74: {       // MOV    M,H
      break;
    }
    case 0x75: {       // MOV    M,L
      break;
    }
    case 0x76: {       // HLT
      break;
    }
    case 0x77: {       // MOV    M,A
      break;
    }
    case 0x78: {       // MOV    A,B
      break;
    }
    case 0x79: {       // MOV    A,C
      break;
    }
    case 0x7a: {       // MOV    A,D
      break;
    }
    case 0x7b: {       // MOV    A,E
      break;
    }
    case 0x7c: {       // MOV    A,H
      break;
    }
    case 0x7d: {       // MOV    A,L
      break;
    }
    case 0x7e: {       // MOV    A,M
      break;
    }
    case 0x7f: {       // MOV    A,A
      break;
    }
    case 0x80: {       // ADD    B
      break;
    }
    case 0x81: {       // ADD    C
      break;
    }
    case 0x82: {       // ADD    D
      break;
    }
    case 0x83: {       // ADD    E
      break;
    }
    case 0x84: {       // ADD    H
      break;
    }
    case 0x85: {       // ADD    L
      break;
    }
    case 0x86: {       // ADD    M
      break;
    }
    case 0x87: {       // ADD    A
      break;
    }
    case 0x88: {       // ADC    B
      break;
    }
    case 0x89: {       // ADC    C
      break;
    }
    case 0x8a: {       // ADC    D
      break;
    }
    case 0x8b: {       // ADC    E
      break;
    }
    case 0x8c: {       // ADC    H
      break;
    }
    case 0x8d: {       // ADC    L
      break;
    }
    case 0x8e: {       // ADC    M
      break;
    }
    case 0x8f: {       // ADC    A
      break;
    }
    case 0x90: {       // SUB    B
      break;
    }
    case 0x91: {       // SUB    C
      break;
    }
    case 0x92: {       // SUB    D
      break;
    }
    case 0x93: {       // SUB    E
      break;
    }
    case 0x94: {       // SUB    H
      break;
    }
    case 0x95: {       // SUB    L
      break;
    }
    case 0x96: {       // SUB    M
      break;
    }
    case 0x97: {       // SUB    A
      break;
    }
    case 0x98: {       // SBB    B
      break;
    }
    case 0x99: {       // SBB    C
      break;
    }
    case 0x9a: {       // SBB    D
      break;
    }
    case 0x9b: {       // SBB    E
      break;
    }
    case 0x9c: {       // SBB    H
      break;
    }
    case 0x9d: {       // SBB    L
      break;
    }
    case 0x9e: {       // SBB    M
      break;
    }
    case 0x9f: {       // SBB    A
      break;
    }
    case 0xa0: {       // ANA    B
      break;
    }
    case 0xa1: {       // ANA    C
      break;
    }
    case 0xa2: {       // ANA    D
      break;
    }
    case 0xa3: {       // ANA    E
      break;
    }
    case 0xa4: {       // ANA    H
      break;
    }
    case 0xa5: {       // ANA    L
      break;
    }
    case 0xa6: {       // ANA    M
      break;
    }
    case 0xa7: {       // ANA    A
      break;
    }
    case 0xa8: {       // XRA    B
      break;
    }
    case 0xa9: {       // XRA    C
      break;
    }
    case 0xaa: {       // XRA    D
      break;
    }
    case 0xab: {       // XRA    E
      break;
    }
    case 0xac: {       // XRA    H
      break;
    }
    case 0xad: {       // XRA    L
      break;
    }
    case 0xae: {       // XRA    M
      break;
    }
    case 0xaf: {       // XRA    A
      break;
    }
    case 0xb0: {       // ORA    B
      break;
    }
    case 0xb1: {       // ORA    C
      break;
    }
    case 0xb2: {       // ORA    D
      break;
    }
    case 0xb3: {       // ORA    E
      break;
    }
    case 0xb4: {       // ORA    H
      break;
    }
    case 0xb5: {       // ORA    L
      break;
    }
    case 0xb6: {       // ORA    M
      break;
    }
    case 0xb7: {       // ORA    A
      break;
    }
    case 0xb8: {       // CMP    B
      break;
    }
    case 0xb9: {       // CMP    C
      break;
    }
    case 0xba: {       // CMP    D
      break;
    }
    case 0xbb: {       // CMP    E
      break;
    }
    case 0xbc: {       // CMP    H
      break;
    }
    case 0xbd: {       // CMP    L
      break;
    }
    case 0xbe: {       // CMP    M
      break;
    }
    case 0xbf: {       // CMP    A
      break;
    }
    case 0xc0: {       // RNZ
      break;
    }
    case 0xc1: {       // POP    B
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
      break;
    }
    case 0xc5: {       // PUSH   B
      break;
    }
    case 0xc6: {       // ADI
      break;
    }
    case 0xc7: {       // RST    0
      break;
    }
    case 0xc8: {       // RZ
      break;
    }
    case 0xc9: {       // RET
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
      break;
    }
    case 0xcd: {       // CALL
      break;
    }
    case 0xce: {       // ACI
      break;
    }
    case 0xcf: {       // RST
      break;
    }
    case 0xd0: {       // RNC
      break;
    }
    case 0xd1: {       // POP    D
      break;
    }
    case 0xd2: {       // JNC
      if (state->cc.CY != 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xd3: {       // OUT
      break;
    }
    case 0xd4: {       // CNC
      break;
    }
    case 0xd5: {       // PUSH   D
      break;
    }
    case 0xd6: {       // SUI
      break;
    }
    case 0xd7: {       // RST
      break;
    }
    case 0xd8: {       // RC
      break;
    }
    case 0xd9: {       // NOP
      break;
    }
    case 0xda: {       // JC
      if (state->cc.Z == 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xdb: {       // IN
      break;
    }
    case 0xdc: {       // CC
      break;
    }
    case 0xdd: {       // NOP
      break;
    }
    case 0xde: {       // SBI
      break;
    }
    case 0xdf: {       // RST
      break;
    }
    case 0xe0: {       // RPO
      break;
    }
    case 0xe1: {       // POP    H
      break;
    }
    case 0xe2: {       // JPO
      if (state->cc.P == 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xe3: {       // XTHL
      break;
    }
    case 0xe4: {       // CPO
      break;
    }
    case 0xe5: {       // PUSH   H
      break;
    }
    case 0xe6: {       // ANI
      break;
    }
    case 0xe7: {       // RST
      break;
    }
    case 0xe8: {       // RPE
      break;
    }
    case 0xe9: {       // PCHL
      break;
    }
    case 0xea: {       // JPE
      if (state->cc.P != 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xeb: {       // XCHG
      break;
    }
    case 0xec: {       // CPE
      break;
    }
    case 0xed: {       // NOP
      break;
    }
    case 0xee: {       // XRI
      break;
    }
    case 0xef: {       // RST
      break;
    }
    case 0xf0: {       // RP
      break;
    }
    case 0xf1: {       // POP    PSW
      break;
    }
    case 0xf2: {       // JP
      if (state->cc.S == 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xf3: {       // DI
      break;
    }
    case 0xf4: {       // CP
      break;
    }
    case 0xf5: {       // PUSH   PSW
      break;
    }
    case 0xf6: {       // ORI
      break;
    }
    case 0xf7: {       // RST
      break;
    }
    case 0xf8: {       // RM
      break;
    }
    case 0xf9: {       // SPHL
      break;
    }
    case 0xfa: {       // JM
      if (state->cc.S != 0) state->PC = (opcode[2] << 8) | opcode[1];
      else state->PC += 2;
      break;
    }
    case 0xfb: {       // EI
      break;
    }
    case 0xfc: {       // CM
      break;
    }
    case 0xfd: {       // NOP
      break;
    }
    case 0xfe: {       // CPI
      break;
    }
    case 0xff: {       // RST
      break;
    }

    default:
      printf("Error: hex '%02x' not recognized!", *opcode);
      break;
  }

  return 0;
}

int main()
{

  return 0;
}