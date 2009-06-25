/*
* Copyright (C) 2007, GrammarSoft ApS
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

#include "Cohort.h"
#include "Set.h"
#include "Tag.h"
#include "CompositeTag.h"
#include "Rule.h"
#include "Anchor.h"
#include "ContextualTest.h"
#include "Reading.h"
#include "Window.h"
#include "SingleWindow.h"

using namespace CG3;

Cohort::Cohort(SingleWindow *p) {
	wordform = 0;
	global_number = 0;
	local_number = 0;
	parent = p;
	dep_done = false;
	is_related = false;
	is_enclosed = false;
	num_is_current = false;
	text_pre = 0;
	text_post = 0;
	dep_self = 0;
	dep_parent = UINT_MAX;
	prev = 0;
	next = 0;
	is_pleft = 0;
	is_pright = 0;
}

void Cohort::clear(SingleWindow *p) {
	foreach (std::list<Reading*>, readings, iter1, iter1_end) {
		delete (*iter1);
	}
	readings.clear();
	foreach (std::list<Reading*>, deleted, iter2, iter2_end) {
		delete (*iter2);
	}
	deleted.clear();
	foreach (std::list<Reading*>, delayed, iter3, iter3_end) {
		delete (*iter3);
	}
	delayed.clear();
	if (parent) {
		parent->parent->cohort_map.erase(global_number);
		parent->parent->dep_window.erase(global_number);
	}

	wordform = 0;
	global_number = 0;
	local_number = 0;
	parent = p;
	dep_done = false;
	is_related = false;
	is_enclosed = false;
	num_is_current = false;
	if (text_pre) {
		delete[] text_pre;
	}
	text_pre = 0;
	if (text_post) {
		delete[] text_post;
	}
	text_post = 0;
	dep_self = 0;
	dep_parent = UINT_MAX;
	is_pleft = 0;
	is_pright = 0;
	possible_sets.clear();
	dep_children.clear();
	enclosed.clear();
	detach();
	num_max.clear();
	num_min.clear();
}

Cohort::~Cohort() {
	foreach (std::list<Reading*>, readings, iter1, iter1_end) {
		delete (*iter1);
	}
	foreach (std::list<Reading*>, deleted, iter2, iter2_end) {
		delete (*iter2);
	}
	foreach (std::list<Reading*>, delayed, iter3, iter3_end) {
		delete (*iter3);
	}
	if (parent) {
		parent->parent->cohort_map.erase(global_number);
		parent->parent->dep_window.erase(global_number);
	}
	if (text_pre) {
		delete[] text_pre;
	}
	if (text_post) {
		delete[] text_post;
	}
	detach();
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

void Cohort::appendReading(Reading *read) {
	readings.push_back(read);
	if (read->number == 0) {
		read->number = (uint32_t)readings.size();
	}
	num_is_current = false;
}

Reading* Cohort::allocateAppendReading() {
	Reading *read = new Reading(this);
	readings.push_back(read);
	if (read->number == 0) {
		read->number = (uint32_t)readings.size();
	}
	num_is_current = false;
	return read;
}

inline void Cohort::updateMinMax() {
	if (num_is_current) {
		return;
	}
	num_min.clear();
	num_max.clear();
	const_foreach(std::list<Reading*>, readings, rter, rter_end) {
		const_foreach(Taguint32HashMap, (*rter)->tags_numerical, nter, nter_end) {
			const Tag *tag = nter->second;
			if (num_min.find(tag->comparison_hash) == num_min.end() || tag->comparison_val < num_min[tag->comparison_hash]) {
				num_min[tag->comparison_hash] = tag->comparison_val;
			}
			if (num_max.find(tag->comparison_hash) == num_max.end() || tag->comparison_val > num_max[tag->comparison_hash]) {
				num_max[tag->comparison_hash] = tag->comparison_val;
			}
		}
	}
	num_is_current = true;
}

int32_t Cohort::getMin(uint32_t key) {
	updateMinMax();
	if (num_min.find(key) != num_min.end()) {
		return num_min[key];
	}
	return INT_MIN;
}

int32_t Cohort::getMax(uint32_t key) {
	updateMinMax();
	if (num_max.find(key) != num_max.end()) {
		return num_max[key];
	}
	return INT_MAX;
}
