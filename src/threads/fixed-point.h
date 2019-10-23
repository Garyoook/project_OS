//
// Created by yg9418 on 20/10/19.
//

#include <stdint.h>

#ifndef PINTOS_47_FIXED_POINT_H
#define PINTOS_47_FIXED_POINT_H

#endif //PINTOS_47_FIXED_POINT_H

typedef int32_t fp;

#define F (1 << 14)

#define fp_is_positive(x) (((x) >> 31) == 0)
#define fp_int_to_fp(n) ((n)*F)
#define fp_x_to_integer_round_to_zero(x) ((x)/F)
#define fp_x_to_integer_round_to_nearest(x) (fp_is_positive(x) ? ((x) + F/2)/F : ((x) - F/2)/F)
#define fp_add(x, y) ((x) + (y))
#define fp_sub(x, y) ((x) - (y))
#define fp_add_fp_and_int(x, n) ((x) + (n)*F)
#define fp_sub_n_from_x(x, n) ((x) - (n)*F)
#define fp_multi(x, y) ((fp)(((int64_t) (x))*(y)/F))
#define fp_multi_fp_int(x, n) ((x)*(n))
#define fp_divide(x, y) ((fp)(((int64_t) (x))*F/(y)))
#define fp_divide_x_by_n(x, n) ((x)/(n))
#define fp_frac(d, n) (fp_divide_x_by_n(fp_int_to_fp(d), n))