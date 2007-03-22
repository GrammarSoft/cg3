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
	number = 0;
	next = 0;
	previous = 0;
	parent = p;
	text = 0;
}

Cohort::~Cohort() {
	std::list<Reading*>::iterator iter;
	for (iter = readings.begin() ; iter != readings.end() ; iter++) {
		delete *iter;
	}
	readings.clear();
	if (next && previous) {
		next->previous = previous;
		previous->next = next;
	}
	else {
		if (next) {
			next->previous = 0;
		}
		if (previous) {
			previous->next = 0;
		}
	}
	parent->parent->cohort_map.erase(number);
}

void Cohort::addParent(uint32_t parent) {
	std::list<Reading*>::iterator iter;
	for (iter = readings.begin() ; iter != readings.end() ; iter++) {
		(*iter)->dep_parents.insert(parent);
	}
}

void Cohort::addSibling(uint32_t sibling) {
	std::list<Reading*>::iterator iter;
	for (iter = readings.begin() ; iter != readings.end() ; iter++) {
		(*iter)->dep_siblings.insert(sibling);
	}
}

void Cohort::remSibling(uint32_t sibling) {
	std::list<Reading*>::iterator iter;
	for (iter = readings.begin() ; iter != readings.end() ; iter++) {
		(*iter)->dep_siblings.erase(sibling);
	}
}

void Cohort::addChild(uint32_t child) {
	std::list<Reading*>::iterator iter;
	for (iter = readings.begin() ; iter != readings.end() ; iter++) {
		(*iter)->dep_children.insert(child);
	}
}

void Cohort::appendReading(Reading *read) {
	read->parent = this;
	readings.push_back(read);
}

void Cohort::reenumerate() {
	Cohort *c = this;
	if (c->previous) {
		c = c->previous;
	}
	while (c->next) {
		c->next->number = c->number+1;
		c = c->next;
	}
	u_fprintf(ux_stderr, "Info: Reenumerate called.\n");
}

Reading* Cohort::allocateAppendReading() {
	Reading *read = new Reading();
	read->parent = this;
	readings.push_back(read);
	return read;
}
