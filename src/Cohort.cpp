/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */
#include "Cohort.h"

using namespace CG3;

Cohort::Cohort(SingleWindow *p) {
	wordform = 0;
	global_number = 0;
	local_number = 0;
	parent = p;
	dep_done = false;
	is_related = false;
	text = 0;
	dep_self = 0;
	dep_parent = 0;
	invalid_rules.clear();
}

void Cohort::clear(SingleWindow *p) {
	Recycler *r = Recycler::instance();
	std::list<Reading*>::iterator iter;
	for (iter = readings.begin() ; iter != readings.end() ; iter++) {
		r->delete_Reading(*iter);
	}
	readings.clear();
	for (iter = deleted.begin() ; iter != deleted.end() ; iter++) {
		r->delete_Reading(*iter);
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
	dep_done = false;
	is_related = false;
	if (text) {
		delete[] text;
	}
	text = 0;
	dep_self = 0;
	dep_parent = 0;
	invalid_rules.clear();
}

Cohort::~Cohort() {
	Recycler *r = Recycler::instance();
	std::list<Reading*>::iterator iter;
	for (iter = readings.begin() ; iter != readings.end() ; iter++) {
		r->delete_Reading(*iter);
	}
	readings.clear();
	for (iter = deleted.begin() ; iter != deleted.end() ; iter++) {
		r->delete_Reading(*iter);
	}
	deleted.clear();
	parent->parent->cohort_map.erase(global_number);
	parent->parent->dep_window.erase(global_number);
	invalid_rules.clear();
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
	readings.push_back(read);
}

Reading* Cohort::allocateAppendReading() {
	Recycler *r = Recycler::instance();
	Reading *read = r->new_Reading(this);
	readings.push_back(read);
	return read;
}
