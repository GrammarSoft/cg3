/*
* Copyright (C) 2007-2023, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Cohort.hpp"
#include "Set.hpp"
#include "Tag.hpp"
#include "Rule.hpp"
#include "ContextualTest.hpp"
#include "Reading.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "pool.hpp"

namespace CG3 {

extern pool<Cohort> pool_cohorts;

Cohort* alloc_cohort(SingleWindow* p) {
	Cohort* c = pool_cohorts.get();
	if (c == 0) {
		c = new Cohort(p);
	}
	else {
		c->parent = p;
	}
	return c;
}

void free_cohort(Cohort*& c) {
	if (c == 0) {
		return;
	}
	pool_cohorts.put(c);
	c = 0;
}

Cohort::Cohort(SingleWindow* p)
  : parent(p)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << VOIDP(this) << " " << __PRETTY_FUNCTION__ << std::endl;
	#endif
}

Cohort::~Cohort() {
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << VOIDP(this) << " " << __PRETTY_FUNCTION__ << ": " << readings.size() << ", " << deleted.size() << ", " << delayed.size() << std::endl;
	#endif

	for (auto iter1 : readings) {
		free_reading(iter1);
	}
	for (auto iter2 : deleted) {
		free_reading(iter2);
	}
	for (auto iter3 : delayed) {
		free_reading(iter3);
	}
	free_reading(wread);

	for (auto iter : removed) {
		free_cohort(iter);
	}
	if (parent) {
		parent->parent->cohort_map.erase(global_number);
		parent->parent->dep_window.erase(global_number);
	}
	detach();
}

void Cohort::clear() {
	if (parent && parent->parent) {
		parent->parent->cohort_map.erase(global_number);
		parent->parent->dep_window.erase(global_number);
	}
	detach();

	type = 0;
	global_number = 0;
	local_number = 0;
	wordform = nullptr;
	dep_self = 0;
	dep_parent = DEP_NO_PARENT;
	is_pleft = 0;
	is_pright = 0;
	parent = nullptr;

	text.clear();
	wblank.clear();
	num_max.clear();
	num_min.clear();
	dep_children.clear();
	possible_sets.clear();
	relations.clear();
	relations_input.clear();

	for (auto iter1 : readings) {
		free_reading(iter1);
	}
	for (auto iter2 : deleted) {
		free_reading(iter2);
	}
	for (auto iter3 : delayed) {
		free_reading(iter3);
	}
	free_reading(wread);

	readings.clear();
	deleted.clear();
	delayed.clear();
	wread = nullptr;

	for (auto iter : removed) {
		free_cohort(iter);
	}
	removed.clear();
	assert(enclosed.empty() && "Enclosed was not empty!");
}

void Cohort::detach() {
	if (prev) {
		prev->next = next;
	}
	if (next) {
		next->prev = prev;
	}
	prev = next = nullptr;
}

void Cohort::addChild(uint32_t child) {
	dep_children.insert(child);
}

void Cohort::remChild(uint32_t child) {
	dep_children.erase(child);
}

void Cohort::appendReading(Reading* read, ReadingList& readings) {
	readings.push_back(read);
	if (read->number == 0) {
		read->number = UI32(readings.size() * 1000 + 1000);
	}
	type &= ~CT_NUM_CURRENT;
}

void Cohort::appendReading(Reading* read) {
	return appendReading(read, readings);
}

Reading* Cohort::allocateAppendReading() {
	Reading* read = alloc_reading(this);
	readings.push_back(read);
	if (read->number == 0) {
		read->number = UI32(readings.size() * 1000 + 1000);
	}
	type &= ~CT_NUM_CURRENT;
	return read;
}

Reading* Cohort::allocateAppendReading(Reading& r) {
	Reading* read = alloc_reading(r);
	readings.push_back(read);
	if (read->number == 0) {
		read->number = UI32(readings.size() * 1000 + 1000);
	}
	type &= ~CT_NUM_CURRENT;
	return read;
}

void Cohort::updateMinMax() {
	if (type & CT_NUM_CURRENT) {
		return;
	}
	num_min.clear();
	num_max.clear();
	for (auto rter : readings) {
		for (const auto& nter : rter->tags_numerical) {
			const Tag* tag = nter.second;
			if (num_min.find(tag->comparison_hash) == num_min.end() || tag->comparison_val < num_min[tag->comparison_hash]) {
				num_min[tag->comparison_hash] = tag->comparison_val;
			}
			if (num_max.find(tag->comparison_hash) == num_max.end() || tag->comparison_val > num_max[tag->comparison_hash]) {
				num_max[tag->comparison_hash] = tag->comparison_val;
			}
		}
	}
	type |= CT_NUM_CURRENT;
}

double Cohort::getMin(uint32_t key) {
	updateMinMax();
	if (num_min.find(key) != num_min.end()) {
		return num_min[key];
	}
	return NUMERIC_MIN;
}

double Cohort::getMax(uint32_t key) {
	updateMinMax();
	if (num_max.find(key) != num_max.end()) {
		return num_max[key];
	}
	return NUMERIC_MAX;
}

bool Cohort::addRelation(uint32_t rel, uint32_t cohort) {
	auto& cohorts = relations[rel];
	const size_t sz = cohorts.size();
	cohorts.insert(cohort);
	return (sz != cohorts.size());
}

bool Cohort::setRelation(uint32_t rel, uint32_t cohort) {
	relations_input.erase(rel);
	auto& cohorts = relations[rel];
	if (cohorts.size() == 1 && cohorts.find(cohort) != cohorts.end()) {
		return false;
	}
	cohorts.clear();
	cohorts.insert(cohort);
	return true;
}

bool Cohort::remRelation(uint32_t rel, uint32_t cohort) {
	auto it = relations.find(rel);
	if (it != relations.end()) {
		auto& rels = it->second;
		const size_t sz = rels.size();
		rels.erase(cohort);
		auto it_in = relations_input.find(rel);
		if (it_in != relations_input.end()) {
			it_in->second.erase(cohort);
		}
		return (sz != rels.size());
	}
	return false;
}

void Cohort::setRelated() {
	type |= CT_RELATED;
	for (auto& r : readings) {
		r->noprint = false;
	}
}
}
