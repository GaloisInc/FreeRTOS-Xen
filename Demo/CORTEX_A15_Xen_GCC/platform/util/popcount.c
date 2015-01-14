
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
