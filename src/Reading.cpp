/*
* Copyright (C) 2007-2010, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Reading.h"

namespace CG3 {

Reading::Reading(Cohort *p) :
mapped(false),
deleted(false),
noprint(false),
matched_target(false),
matched_tests(false),
wordform(0),
baseform(0),
hash(0),
hash_plain(0),
number(0),
tags_bloom(0),
tags_plain_bloom(0),
tags_textual_bloom(0),
mapping(0),
parent(p)
{
	// Nothing in the actual body...
}

Reading::Reading(const Reading& r) :
mapped(r.mapped),
deleted(r.deleted),
noprint(r.noprint),
matched_target(false),
matched_tests(false),
wordform(r.wordform),
baseform(r.baseform),
hash(r.hash),
hash_plain(r.hash_plain),
number(r.number),
tags_bloom(r.tags_bloom),
tags_plain_bloom(r.tags_plain_bloom),
tags_textual_bloom(r.tags_textual_bloom),
mapping(r.mapping),
parent(r.parent),
hit_by(r.hit_by),
tags_list(r.tags_list),
tags(r.tags),
tags_plain(r.tags_plain),
tags_textual(r.tags_textual),
tags_numerical(r.tags_numerical)
{
	// Nothing in the actual body...
}

uint32_t Reading::rehash() {
	hash = 0;
	hash_plain = 0;
	uint32SortedVector::const_iterator iter;
	for (iter = tags.begin() ; iter != tags.end() ; iter++) {
		if (!mapping || mapping->hash != *iter) {
			hash = hash_sdbm_uint32_t(*iter, hash);
		}
	}
	hash_plain = hash;
	if (mapping) {
		hash = hash_sdbm_uint32_t(mapping->hash, hash);
	}
	return hash;
}

bool Reading::cmp_number(Reading *a, Reading *b) {
	return a->number < b->number;
}

}
