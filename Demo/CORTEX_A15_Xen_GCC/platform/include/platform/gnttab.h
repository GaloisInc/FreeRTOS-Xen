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

#ifndef __GNTTAB_H__
#define __GNTTAB_H__

#include <xen/grant_table.h>
#include <freertos/arch_mm.h>

#define NR_RESERVED_ENTRIES 8

/* NR_GRANT_FRAMES must be less than or equal to that configured in Xen */
#define NR_GRANT_FRAMES 4
#define NR_GRANT_ENTRIES (NR_GRANT_FRAMES * PAGE_SIZE / sizeof(grant_entry_t))

void init_gnttab(void);
grant_ref_t gnttab_grant_access(domid_t domid, unsigned long frame,
				int readonly);
int gnttab_map_ref(domid_t domid, grant_ref_t ref, unsigned long mfn);
grant_ref_t gnttab_grant_transfer(domid_t domid, unsigned long pfn);
unsigned long gnttab_end_transfer(grant_ref_t gref);
int gnttab_end_access(grant_ref_t ref);
const char *gnttabop_error(int16_t status);
void fini_gnttab(void);

#endif /* !__GNTTAB_H__ */
