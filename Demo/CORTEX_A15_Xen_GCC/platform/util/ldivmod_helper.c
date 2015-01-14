/*
 * Copyright (C) 2012 Andrew Turner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <freertos/extra-types.h>

u_quad_t __qdivrem(u_quad_t u, u_quad_t v, u_quad_t *rem);

quad_t
__divdi3(quad_t a, quad_t b)
{
        u_quad_t ua, ub, uq;
        int neg;

        if (a < 0)
                ua = -(u_quad_t)a, neg = 1;
        else
                ua = a, neg = 0;
        if (b < 0)
                ub = -(u_quad_t)b, neg ^= 1;
        else
                ub = b;
        uq = __qdivrem(ua, ub, (u_quad_t *)0);
        return (neg ? -uq : uq);
}

/*
 * Helper for __aeabi_ldivmod.
 * TODO: __divdi3 calls __qdivrem. We should do the same and use the
 * remainder value rather than re-calculating it.
 */
long long __kern_ldivmod(long long, long long, long long *);

long long
__kern_ldivmod(long long n, long long m, long long *rem)
{
	long long q;

	q = __divdi3(n, m);	/* q = n / m */
	*rem = n - m * q;

	return q;
}
