/*
 * Copyright (C) 2014-2015 Galois, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License** as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.

 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


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
