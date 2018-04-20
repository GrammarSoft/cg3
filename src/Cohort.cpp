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

#include "Cohort.hpp"
#include "Set.hpp"
#include "Tag.hpp"
#include "Rule.hpp"
#include "ContextualTest.hpp"
#include "Reading.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"

namespace CG3 {

CohortVector pool_cohorts;
pool_cleaner<CohortVector> cleaner_cohorts(pool_cohorts);

Cohort* alloc_cohort(SingleWindow* p) {
	Cohort* c = pool_get(pool_cohorts);
	if (c == 0) {
		c = new Cohort(p);
	}
	else {
		c->parent = p;
	}
	return c;
}

void free_cohort(Cohort* c) {
	if (c == 0) {
		return;
	}
	pool_put(pool_cohorts, c);
}

Cohort::Cohort(SingleWindow* p)
  : type(0)
  , global_number(0)
  , local_number(0)
  , wordform(0)
  , dep_self(0)
  , dep_parent(DEP_NO_PARENT)
  , is_pleft(0)
  , is_pright(0)
  , parent(p)
  , prev(0)
  , next(0)
  , wread(0)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	#endif
}

Cohort::~Cohort() {
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << ": " << readings.size() << ", " << deleted.size() << ", " << delayed.size() << std::endl;
	#endif

	for (auto iter1 : readings) {
		delete (iter1);
	}
	for (auto iter2 : deleted) {
		delete (iter2);
	}
	for (auto iter3 : delayed) {
		delete (iter3);
	}
	delete wread;

	for (auto iter : removed) {
		delete (iter);
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
	wordform = 0;
	dep_self = 0;
	dep_parent = DEP_NO_PARENT;
	is_pleft = 0;
	is_pright = 0;
	parent = 0;

	text.clear();
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
	wread = 0;

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
	prev = next = 0;
}

void Cohort::addChild(uint32_t child) {
	dep_children.insert(child);
}

void Cohort::remChild(uint32_t child) {
	dep_children.erase(child);
}

void Cohort::appendReading(Reading* read) {
	readings.push_back(read);
	if (read->number == 0) {
		read->number = (uint32_t)readings.size() * 1000 + 1000;
	}
	type &= ~CT_NUM_CURRENT;
}

Reading* Cohort::allocateAppendReading() {
	Reading* read = alloc_reading(this);
	readings.push_back(read);
	if (read->number == 0) {
		read->number = (uint32_t)readings.size() * 1000 + 1000;
	}
	type &= ~CT_NUM_CURRENT;
	return read;
}

Reading* Cohort::allocateAppendReading(Reading& r) {
	Reading* read = alloc_reading(r);
	readings.push_back(read);
	if (read->number == 0) {
		read->number = (uint32_t)readings.size() * 1000 + 1000;
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
		for (auto nter : rter->tags_numerical) {
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
	auto& cohorts = relations[rel];
	if (cohorts.size() == 1 && cohorts.find(cohort) != cohorts.end()) {
		return false;
	}
	cohorts.clear();
	cohorts.insert(cohort);
	return true;
}

bool Cohort::remRelation(uint32_t rel, uint32_t cohort) {
	if (relations.find(rel) != relations.end()) {
		const size_t sz = relations.find(rel)->second.size();
		relations.find(rel)->second.erase(cohort);
		return (sz != relations.find(rel)->second.size());
	}
	return false;
}
}
