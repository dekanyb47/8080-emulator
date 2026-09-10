#include <stdint.h>

typedef struct MachineState{
  uint8_t shift_offset;
  uint8_t shift0;
  uint8_t shift1;
  uint8_t *inp_ports;   // numbered 0, 1, 2
  uint8_t *out_ports;   // numbered 2, 3, 5, 6
  uint32_t last_interrupt;
  uint8_t done;
} MachineState;

MachineState *init_machine_state();
void handle_8080_instruction(EmulatorState *es, MachineState *ms);
void machine_key_down(MachineState *ms, int sym);
void machine_key_up(MachineState *ms, int sym);
void free_machine_state(MachineState *ms);
