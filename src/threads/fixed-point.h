//
// Created by jy4917 on 18/10/19.
//

#define F 1 << 14

typedef int32_t fp;

#define N_TO_FIXED_POINT(n) n*F
#define X_TO_INTEGER(x) x/F
#define X_TO_NEAREST_INTEGER(x) if (x>=0) then (x+F/2)/F else (x-F/2)/F
#define ADD(x, y) x+y
#define SUBTRACT(x, y) X-y
#define ADD_WITH_N(x, n) x+n*F
#define SUBTRACT_WITH_N(x, n) x-n*F
#define MULTIPLY(x, y) ((int64_t) x)*y/F
#define MULTIPLY_WITH_N(x, n) x*n
#define DIVIDE(x, y) ((int64_t) x)*F/y
#define DIVIDE_WITH_N(x, n) x/n