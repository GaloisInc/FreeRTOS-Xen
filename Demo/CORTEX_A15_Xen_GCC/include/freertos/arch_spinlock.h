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



#ifndef __ARCH_ASM_SPINLOCK_H
#define __ARCH_ASM_SPINLOCK_H

#include "os.h"


#define ARCH_SPIN_LOCK_UNLOCKED { 1 }

/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */

#define arch_spin_is_locked(x)	(*(volatile signed char *)(&(x)->slock) <= 0)
#define arch_spin_unlock_wait(x) do { barrier(); } while(spin_is_locked(x))

/*
 * This works. Despite all the confusion.
 * (except on PPro SMP or if we are using OOSTORE)
 * (PPro errata 66, 92)
 */

static inline void _raw_spin_unlock(spinlock_t *lock)
{
	xchg(&lock->slock, 1);
}

static inline int _raw_spin_trylock(spinlock_t *lock)
{
	return xchg(&lock->slock, 0) != 0 ? 1 : 0;
}

static inline void _raw_spin_lock(spinlock_t *lock)
{
	volatile int was_locked;
	do {
		was_locked = xchg(&lock->slock, 0) == 0 ? 1 : 0;
	} while(was_locked);
}

static inline void _raw_spin_lock_flags (spinlock_t *lock, unsigned long flags)
{
}

#endif
