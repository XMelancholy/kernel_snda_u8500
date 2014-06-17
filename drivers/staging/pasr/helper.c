/*
 * Copyright (C) ST-Ericsson SA 2011
 * Author: Maxime Coquelin <maxime.coquelin@stericsson.com> for ST-Ericsson.
 * License terms:  GNU General Public License (GPL), version 2
 */

#include <linux/pasr.h>


struct pasr_die *pasr_addr2die(struct pasr_map *map, phys_addr_t addr)
{
	int i;

	if (!map)
		return NULL;

	for (i = 0; i < map->nr_dies; i++) {
		phys_addr_t start;
		struct pasr_die *d = &map->die[i];

		start = addr & ~((PASR_SECTION_SZ * d->nr_sections) - 1);

		if (start == d->start)
			return d;
	}

	return NULL;
}

struct pasr_section *pasr_addr2section(struct pasr_map *map
				, phys_addr_t addr)
{
	unsigned int left, right, mid;
	struct pasr_die *die;

	/* Find the die the address it is located in */
	die = pasr_addr2die(map, addr);
	if (!die)
		goto err;

	left = 0;
	right = die->nr_sections;

	addr &= ~(PASR_SECTION_SZ - 1);

	 while (left != right) {
		struct pasr_section *s;

		mid = (left + right) >> 1;
		s = &die->section[mid];

		if (addr == s->start)
			return s;
		else if (addr > s->start)
			left = mid;
		else
			right = mid;
	}

err:
	/* Provided address isn't in any declared section */
	pr_err("%s: No section found for address %#x",
			__func__, addr);

	return NULL;
}

phys_addr_t pasr_section2addr(struct pasr_section *s)
{
	return s->start;
}
