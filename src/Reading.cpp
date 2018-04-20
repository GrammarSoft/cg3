/*
* Copyright (C) 2007-2018, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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

#include "Reading.hpp"
#include "Cohort.hpp"

namespace CG3 {

ReadingList pool_readings;
pool_cleaner<ReadingList> cleaner_readings(pool_readings);

Reading* alloc_reading(Cohort* p) {
	Reading* r = pool_get(pool_readings);
	if (r == 0) {
		r = new Reading(p);
	}
	else {
		r->number = p ? (p->readings.size() * 1000 + 1000) : 0;
		r->parent = p;
	}
	return r;
}

Reading* alloc_reading(const Reading& o) {
	Reading* r = pool_get(pool_readings);
	if (r == 0) {
		r = new Reading(o);
	}
	else {
		r->mapped = o.mapped;
		r->deleted = o.deleted;
		r->noprint = o.noprint;
		r->matched_target = false;
		r->matched_tests = false;
		r->immutable = false;
		r->baseform = o.baseform;
		r->hash = o.hash;
		r->hash_plain = o.hash_plain;
		r->number = o.number + 100;
		r->tags_bloom = o.tags_bloom;
		r->tags_plain_bloom = o.tags_plain_bloom;
		r->tags_textual_bloom = o.tags_textual_bloom;
		r->mapping = o.mapping;
		r->parent = o.parent;
		r->next = o.next;
		r->hit_by = o.hit_by;
		r->tags_list = o.tags_list;
		r->tags = o.tags;
		r->tags_plain = o.tags_plain;
		r->tags_textual = o.tags_textual;
		r->tags_numerical = o.tags_numerical;
		r->tags_string = o.tags_string;
		r->tags_string_hash = o.tags_string_hash;
		if (r->next) {
			r->next = alloc_reading(*r->next);
		}
	}
	return r;
}

void free_reading(Reading* r) {
	if (r == 0) {
		return;
	}
	pool_put(pool_readings, r);
}

Reading::Reading(Cohort* p)
  : mapped(false)
  , deleted(false)
  , noprint(false)
  , matched_target(false)
  , matched_tests(false)
  , immutable(false)
  , baseform(0)
  , hash(0)
  , hash_plain(0)
  , number(p ? (p->readings.size() * 1000 + 1000) : 0)
  , mapping(0)
  , parent(p)
  , next(0)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	#endif
}

Reading::Reading(const Reading& r)
  : mapped(r.mapped)
  , deleted(r.deleted)
  , noprint(r.noprint)
  , matched_target(false)
  , matched_tests(false)
  , immutable(r.immutable)
  , baseform(r.baseform)
  , hash(r.hash)
  , hash_plain(r.hash_plain)
  , number(r.number + 100)
  , tags_bloom(r.tags_bloom)
  , tags_plain_bloom(r.tags_plain_bloom)
  , tags_textual_bloom(r.tags_textual_bloom)
  , mapping(r.mapping)
  , parent(r.parent)
  , next(r.next)
  , hit_by(r.hit_by)
  , tags_list(r.tags_list)
  , tags(r.tags)
  , tags_plain(r.tags_plain)
  , tags_textual(r.tags_textual)
  , tags_numerical(r.tags_numerical)
  , tags_string(r.tags_string)
  , tags_string_hash(r.tags_string_hash)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	#endif

	if (next) {
		next = allocateReading(*next);
	}
}

Reading::~Reading() {
	#ifdef CG_TRACE_OBJECTS
		std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << ": " << tags.size() << ", " << hit_by.size() << std::endl;
	#endif

	delete next;
	next = 0;
}

void Reading::clear() {
	mapped = false;
	deleted = false;
	noprint = false;
	matched_target = false;
	matched_tests = false;
	immutable = false;
	baseform = 0;
	hash = 0;
	hash_plain = 0;
	number = 0;
	tags_bloom.clear();
	tags_plain_bloom.clear();
	tags_textual_bloom.clear();
	mapping = 0;
	parent = 0;
	free_reading(next);
	next = 0;
	hit_by.clear();
	tags_list.clear();
	tags.clear();
	tags_plain.clear();
	tags_textual.clear();
	tags_numerical.clear();
	tags_string.clear();
	tags_string_hash = 0;
}

Reading* Reading::allocateReading(Cohort* p) {
	return alloc_reading(p);
}

Reading* Reading::allocateReading(const Reading& r) {
	return alloc_reading(r);
}

uint32_t Reading::rehash() {
	hash = 0;
	hash_plain = 0;
	for (auto iter : tags) {
		if (!mapping || mapping->hash != iter) {
			hash = hash_value(iter, hash);
		}
	}
	hash_plain = hash;
	if (mapping) {
		hash = hash_value(mapping->hash, hash);
	}
	if (next) {
		next->rehash();
		hash = hash_value(next->hash, hash);
	}
	return hash;
}

bool Reading::cmp_number(Reading* a, Reading* b) {
	return a->number < b->number;
}
}
