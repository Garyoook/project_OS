//
// Created by yg9418 on 21/10/19.
//
#include "debug.h"
#include "fixed-point.h"


int main(void) {
  ASSERT(fp_x_to_integer_round_to_nearest(fp_int_to_fp(4)) == 4);
  ASSERT(fp_x_to_integer_round_to_zero(fp_int_to_fp(4)) == 4);
  ASSERT(fp_frac(4, 20) == fp_frac(1, 5));
  return 0;
}