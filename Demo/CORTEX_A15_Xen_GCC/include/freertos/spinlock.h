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

#ifndef __ASM_SPINLOCK_H
#define __ASM_SPINLOCK_H

/*
 * Your basic SMP spinlocks, allowing only a single CPU anywhere
 */

typedef struct {
	volatile unsigned int slock;
} spinlock_t;


#include "arch_spinlock.h"


#define SPINLOCK_MAGIC	0xdead4ead

#define SPIN_LOCK_UNLOCKED ARCH_SPIN_LOCK_UNLOCKED

#define spin_lock_init(x)	do { *(x) = SPIN_LOCK_UNLOCKED; } while(0)

/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */

#define spin_is_locked(x)	arch_spin_is_locked(x)

#define spin_unlock_wait(x)	arch_spin_unlock_wait(x)


#define _spin_trylock(lock)     ({_raw_spin_trylock(lock) ? \
                                1 : ({ 0;});})

#define _spin_lock(lock)        \
do {                            \
        _raw_spin_lock(lock);   \
} while(0)

#define _spin_unlock(lock)      \
do {                            \
        _raw_spin_unlock(lock); \
} while (0)


#define spin_lock(lock)       _spin_lock(lock)
#define spin_unlock(lock)       _spin_unlock(lock)

#define DEFINE_SPINLOCK(x) spinlock_t x = SPIN_LOCK_UNLOCKED

#endif
