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
#include "SingleWindow.h"

using namespace CG3;

SingleWindow::SingleWindow(Window *p) {
	text = 0;
	next = 0;
	previous = 0;
	number = 0;
	parent = p;
}

SingleWindow::~SingleWindow() {
	std::vector<Cohort*>::iterator iter;
	for (iter = cohorts.begin() ; iter != cohorts.end() ; iter++) {
		delete *iter;
	}
	cohorts.clear();
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
	parent->window_map.erase(number);
}

void SingleWindow::appendCohort(Cohort *cohort) {
	cohort->parent = this;
	cohort->local_number = (uint32_t)cohorts.size();
	cohorts.push_back(cohort);
	parent->cohort_map[cohort->global_number] = cohort;
}
