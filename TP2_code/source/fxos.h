#ifndef FXOS_H_
#define FXOS_H_

#include <stdint.h>
#include <stdbool.h>

bool FXOS_INIT(void);

void FXOS_service(void);

int16_t get_roll(void);
int16_t get_pitch(void);
int16_t get_heading(void);

void get_orientation(int16_t * roll, int16_t * pitch, int16_t * heading);

#endif /* FXOS_H_ */
