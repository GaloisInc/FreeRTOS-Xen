
#ifndef _EXTRA_TYPES_H
#define _EXTRA_TYPES_H

typedef long long           quad_t;
typedef unsigned long long  u_quad_t;
typedef unsigned long       u_long;

#define _QUAD_HIGHWORD 1
#define _QUAD_LOWWORD 0

#define H               _QUAD_HIGHWORD
#define L               _QUAD_LOWWORD

#define HHALF(x)        ((x) >> HALF_BITS)
#define LHALF(x)        ((x) & ((1 << HALF_BITS) - 1))

#endif
