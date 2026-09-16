// SPDX-FileCopyrightText: 2025 IObundle
//
// SPDX-License-Identifier: MIT

#include <cstdio>

#include "versat_accel.h"

int main(){
  versat_init(0); // PC emulation has no address space to map, so any base address works.

  accelConfig->a.constant = 10;
  accelConfig->b.constant = 5;

  RunAccelerator(1);
  printf("After 1 run:  %d\n", accelState->result.currentValue);

  RunAccelerator(1);
  printf("After 2 runs: %d\n", accelState->result.currentValue);

  return 0;
}
