#include <stdint.h>

typedef struct ConditionCodes {
  uint8_t pad : 3;
  uint8_t AC : 1;
  uint8_t CY : 1;
  uint8_t P : 1;
  uint8_t S : 1;
  uint8_t Z : 1;
} ConditionCodes;

typedef struct EmulatorState {
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
} EmulatorState;

int emulate_8080_op(EmulatorState *state);
EmulatorState *init_emulator_state(char filepath[]);
