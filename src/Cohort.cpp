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

using namespace CG3;

Cohort::Cohort(SingleWindow *p) {
	wordform = 0;
	global_number = 0;
	local_number = 0;
	parent = p;
	is_disamb = false;
	dep_done = false;
	is_related = false;
	text = 0;
	dep_self = 0;
	dep_parent = 0;
}

void Cohort::clear(SingleWindow *p) {
	Recycler *r = Recycler::instance();
	foreach (std::list<Reading*>, readings, iter1, iter1_end) {
		r->delete_Reading(*iter1);
	}
	readings.clear();
	foreach (std::list<Reading*>, deleted, iter2, iter2_end) {
		r->delete_Reading(*iter2);
	}
	deleted.clear();
	if (parent) {
		parent->parent->cohort_map.erase(global_number);
		parent->parent->dep_window.erase(global_number);
	}

	wordform = 0;
	global_number = 0;
	local_number = 0;
	parent = p;
	is_disamb = false;
	dep_done = false;
	is_related = false;
	if (text) {
		delete[] text;
	}
	text = 0;
	dep_self = 0;
	dep_parent = 0;
	possible_sets.clear();
}

Cohort::~Cohort() {
	Recycler *r = Recycler::instance();
	foreach (std::list<Reading*>, readings, iter1, iter1_end) {
		r->delete_Reading(*iter1);
	}
	foreach (std::list<Reading*>, deleted, iter2, iter2_end) {
		r->delete_Reading(*iter2);
	}
	if (parent) {
		parent->parent->cohort_map.erase(global_number);
		parent->parent->dep_window.erase(global_number);
	}
	if (text) {
		delete[] text;
	}
}

void Cohort::addSibling(uint32_t sibling) {
	dep_siblings.insert(sibling);
}

void Cohort::remSibling(uint32_t sibling) {
	dep_siblings.erase(sibling);
}

void Cohort::addChild(uint32_t child) {
	dep_children.insert(child);
}

void Cohort::remChild(uint32_t child) {
	dep_children.erase(child);
}

void Cohort::appendReading(Reading *read) {
	assert(read);
	readings.push_back(read);
	if (read->number == 0) {
		read->number = (uint32_t)readings.size();
	}
}

Reading* Cohort::allocateAppendReading() {
	Recycler *r = Recycler::instance();
	Reading *read = r->new_Reading(this);
	readings.push_back(read);
	if (read->number == 0) {
		read->number = (uint32_t)readings.size();
	}
	return read;
}
