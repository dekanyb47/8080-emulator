#include <stdio.h>
#include <stdint.h>

typedef struct ConditionCodes {
  uint8_t S:1;
  uint8_t Z:1;
  uint8_t AC:1;
  uint8_t P:1; 
  uint8_t CY:1;
  uint8_t pad:3;
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

void unimplentedInstruction() {
  printf("Unimplemented instruction!\n");
  exit(1);
}

int emulate_8080(State8080 *state) {
  unsigned char *opcode = &state->memory[state->PC];

  switch(*opcode) {
    case 0x00: break;   // NOP
    case 0x01:          // LXI    B
      state->C = opcode[1];
      state->B = opcode[2];
      state->PC += 2;
      break;
    case 0x02:          // STAX   B
      state->C = 0x00;
      state->B = state->A;
      break;
    case 0x03:          // INX    B
      state->C += 1;
      if (state->C == 0) state->B++;
      break;
    case 0x04:          // INR    B
      uint8_t res = state->B + 1;

      state->cc.Z = (res == 0);
      state->cc.S;

      state->B = res;
    
      break;
    case 0x05: break;   // DCR    B
    case 0x06: break;   // MVI    B
    case 0x07: break;   // RLC
    case 0x08: break;   // NOP
    case 0x09: break;   // DAD    B
    case 0x0a: break;   // LDAX   B
    case 0x0b: break;   // DCX    B
    case 0x0c: break;   // INR    C
    case 0x0d: break;   // DCR    C
    case 0x0e: break;   // MVI    C
    case 0x0f: break;   // RRC
    case 0x10: break;   // NOP
    case 0x11: break;   // LXI    D
    case 0x12: break;   // STAX   D
    case 0x13: break;   // INX    D
    case 0x14: break;   // INR    D
    case 0x15: break;   // DCR    D
    case 0x16: break;   // MVI    D
    case 0x17: break;   // RAL
    case 0x18: break;   // NOP
    case 0x19: break;   // DAD    D
    case 0x1a: break;   // LDAX   D
    case 0x1b: break;   // DCX    D
    case 0x1c: break;   // INR    E
    case 0x1d: break;   // DCR    E
    case 0x1e: break;   // MVI    E
    case 0x1f: break;   // RAR
    case 0x20: break;   // NOP
    case 0x21: break;   // LXI    H
    case 0x22: break;   // SHLD
    case 0x23: break;   // INX    H
    case 0x24: break;   // INR    H
    case 0x25: break;   // DCR    H
    case 0x26: break;   // MVI    H
    case 0x27: break;   // DAA
    case 0x28: break;   // NOP
    case 0x29: break;   // DAD    H
    case 0x2a: break;   // LHLD
    case 0x2b: break;   // DCX    H
    case 0x2c: break;   // INR    L
    case 0x2d: break;   // DCR    L
    case 0x2e: break;   // MVI    L
    case 0x2f: break;   // CMA
    case 0x30: break;   // NOP
    case 0x31: break;   // LXI    SP
    case 0x32: break;   // STA
    case 0x33: break;   // INX    SP
    case 0x34: break;   // INR    M
    case 0x35: break;   // DCR    M
    case 0x36: break;   // MVI    M
    case 0x37: break;   // STC
    case 0x38: break;   // NOP
    case 0x39: break;   // DAD    SP
    case 0x3a: break;   // LDA
    case 0x3b: break;   // DCX    SP
    case 0x3c: break;   // INR    A
    case 0x3d: break;   // DCR    A
    case 0x3e: break;   // MVI    A
    case 0x3f: break;   // CMC
    case 0x40: break;   // MOV    B,B
    case 0x41: break;   // MOV    B,C
    case 0x42: break;   // MOV    B,D
    case 0x43: break;   // MOV    B,E
    case 0x44: break;   // MOV    B,H
    case 0x45: break;   // MOV    B,L
    case 0x46: break;   // MOV    B,M
    case 0x47: break;   // MOV    B,A
    case 0x48: break;   // MOV    C,B
    case 0x49: break;   // MOV    C,C
    case 0x4a: break;   // MOV    C,D
    case 0x4b: break;   // MOV    C,E
    case 0x4c: break;   // MOV    C,H
    case 0x4d: break;   // MOV    C,L
    case 0x4e: break;   // MOV    C,M
    case 0x4f: break;   // MOV    C,A
    case 0x50: break;   // MOV    D,B
    case 0x51: break;   // MOV    D,C
    case 0x52: break;   // MOV    D,D
    case 0x53: break;   // MOV    D,E
    case 0x54: break;   // MOV    D,H
    case 0x55: break;   // MOV    D,L
    case 0x56: break;   // MOV    D,M
    case 0x57: break;   // MOV    D,A
    case 0x58: break;   // MOV    E,B
    case 0x59: break;   // MOV    E,C
    case 0x5a: break;   // MOV    E,D
    case 0x5b: break;   // MOV    E,E
    case 0x5c: break;   // MOV    E,H
    case 0x5d: break;   // MOV    E,L
    case 0x5e: break;   // MOV    E,M
    case 0x5f: break;   // MOV    E,A
    case 0x60: break;   // MOV    H,B
    case 0x61: break;   // MOV    H,C
    case 0x62: break;   // MOV    H,D
    case 0x63: break;   // MOV    H,E
    case 0x64: break;   // MOV    H,H
    case 0x65: break;   // MOV    H,L
    case 0x66: break;   // MOV    H,M
    case 0x67: break;   // MOV    H,A
    case 0x68: break;   // MOV    L,B
    case 0x69: break;   // MOV    L,C
    case 0x6a: break;   // MOV    L,D
    case 0x6b: break;   // MOV    L,E
    case 0x6c: break;   // MOV    L,H
    case 0x6d: break;   // MOV    L,L
    case 0x6e: break;   // MOV    L,M
    case 0x6f: break;   // MOV    L,A
    case 0x70: break;   // MOV    M,B
    case 0x71: break;   // MOV    M,C
    case 0x72: break;   // MOV    M,D
    case 0x73: break;   // MOV    M,E
    case 0x74: break;   // MOV    M,H
    case 0x75: break;   // MOV    M,L
    case 0x76: break;   // HLT
    case 0x77: break;   // MOV    M,A
    case 0x78: break;   // MOV    A,B
    case 0x79: break;   // MOV    A,C
    case 0x7a: break;   // MOV    A,D
    case 0x7b: break;   // MOV    A,E
    case 0x7c: break;   // MOV    A,H
    case 0x7d: break;   // MOV    A,L
    case 0x7e: break;   // MOV    A,M
    case 0x7f: break;   // MOV    A,A
    case 0x80: break;   // ADD    B
    case 0x81: break;   // ADD    C
    case 0x82: break;   // ADD    D
    case 0x83: break;   // ADD    E
    case 0x84: break;   // ADD    H
    case 0x85: break;   // ADD    L
    case 0x86: break;   // ADD    M
    case 0x87: break;   // ADD    A
    case 0x88: break;   // ADC    B
    case 0x89: break;   // ADC    C
    case 0x8a: break;   // ADC    D
    case 0x8b: break;   // ADC    E
    case 0x8c: break;   // ADC    H
    case 0x8d: break;   // ADC    L
    case 0x8e: break;   // ADC    M
    case 0x8f: break;   // ADC    A
    case 0x90: break;   // SUB    B
    case 0x91: break;   // SUB    C
    case 0x92: break;   // SUB    D
    case 0x93: break;   // SUB    E
    case 0x94: break;   // SUB    H
    case 0x95: break;   // SUB    L
    case 0x96: break;   // SUB    M
    case 0x97: break;   // SUB    A
    case 0x98: break;   // SBB    B
    case 0x99: break;   // SBB    C
    case 0x9a: break;   // SBB    D
    case 0x9b: break;   // SBB    E
    case 0x9c: break;   // SBB    H
    case 0x9d: break;   // SBB    L
    case 0x9e: break;   // SBB    M
    case 0x9f: break;   // SBB    A
    case 0xa0: break;   // ANA    B
    case 0xa1: break;   // ANA    C
    case 0xa2: break;   // ANA    D
    case 0xa3: break;   // ANA    E
    case 0xa4: break;   // ANA    H
    case 0xa5: break;   // ANA    L
    case 0xa6: break;   // ANA    M
    case 0xa7: break;   // ANA    A
    case 0xa8: break;   // XRA    B
    case 0xa9: break;   // XRA    C
    case 0xaa: break;   // XRA    D
    case 0xab: break;   // XRA    E
    case 0xac: break;   // XRA    H
    case 0xad: break;   // XRA    L
    case 0xae: break;   // XRA    M
    case 0xaf: break;   // XRA    A
    case 0xb0: break;   // ORA    B
    case 0xb1: break;   // ORA    C
    case 0xb2: break;   // ORA    D
    case 0xb3: break;   // ORA    E
    case 0xb4: break;   // ORA    H
    case 0xb5: break;   // ORA    L
    case 0xb6: break;   // ORA    M
    case 0xb7: break;   // ORA    A
    case 0xb8: break;   // CMP    B
    case 0xb9: break;   // CMP    C
    case 0xba: break;   // CMP    D
    case 0xbb: break;   // CMP    E
    case 0xbc: break;   // CMP    H
    case 0xbd: break;   // CMP    L
    case 0xbe: break;   // CMP    M
    case 0xbf: break;   // CMP    A
    case 0xc0: break;   // RNZ
    case 0xc1: break;   // POP    B
    case 0xc2: break;   // JNZ
    case 0xc3: break;   // JMP
    case 0xc4: break;   // CNZ
    case 0xc5: break;   // PUSH   B
    case 0xc6: break;   // ADI
    case 0xc7: break;   // RST    0
    case 0xc8: break;   // RZ
    case 0xc9: break;   // RET
    case 0xca: break;   // JZ
    case 0xcb: break;   // NOP
    case 0xcc: break;   // CZ
    case 0xcd: break;   // CALL
    case 0xce: break;   // ACI
    case 0xcf: break;   // RST
    case 0xd0: break;   // RNC
    case 0xd1: break;   // POP    D
    case 0xd2: break;   // JNC
    case 0xd3: break;   // OUT
    case 0xd4: break;   // CNC
    case 0xd5: break;   // PUSH   D
    case 0xd6: break;   // SUI
    case 0xd7: break;   // RST
    case 0xd8: break;   // RC
    case 0xd9: break;   // NOP
    case 0xda: break;   // JC
    case 0xdb: break;   // IN
    case 0xdc: break;   // CC
    case 0xdd: break;   // NOP
    case 0xde: break;   // SBI
    case 0xdf: break;   // RST
    case 0xe0: break;   // RPO
    case 0xe1: break;   // POP    H
    case 0xe2: break;   // JPO
    case 0xe3: break;   // XTHL
    case 0xe4: break;   // CPO
    case 0xe5: break;   // PUSH   H
    case 0xe6: break;   // ANI
    case 0xe7: break;   // RST
    case 0xe8: break;   // RPE
    case 0xe9: break;   // PCHL
    case 0xea: break;   // JPE
    case 0xeb: break;   // XCHG
    case 0xec: break;   // CPE
    case 0xed: break;   // NOP
    case 0xee: break;   // XRI
    case 0xef: break;   // RST
    case 0xf0: break;   // RP
    case 0xf1: break;   // POP    PSW
    case 0xf2: break;   // JP
    case 0xf3: break;   // DI
    case 0xf4: break;   // CP
    case 0xf5: break;   // PUSH   PSW
    case 0xf6: break;   // ORI
    case 0xf7: break;   // RST
    case 0xf8: break;   // RM
    case 0xf9: break;   // SPHL
    case 0xfa: break;   // JM
    case 0xfb: break;   // EI
    case 0xfc: break;   // CM
    case 0xfd: break;   // NOP
    case 0xfe: break;   // CPI
    case 0xff: break;   // RST
    
    default: printf("Error: hex '%02x' not recognized!", *opcode); break;
  }
}

int main() {

  return 0;
}