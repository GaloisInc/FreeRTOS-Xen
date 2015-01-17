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


#include <stdint.h>

// Slow but sufficient.  We need this because we lose access to
// __builtin_popcount when we start compiling with -nostdlib.
uint32_t popcount(uint32_t n)
{
	int c, i;
	c = 0;
	for (i = 0; i < 32; i++) {
		if (n & 0x1)
			c++;
		n >>= 1;
	}
	return c;
}
