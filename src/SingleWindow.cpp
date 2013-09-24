/*
* Copyright (C) 2007-2013, GrammarSoft ApS
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

#include "SingleWindow.hpp"
#include "GrammarApplicator.hpp"
#include "Window.hpp"

namespace CG3 {

SingleWindow::SingleWindow(Window *p) :
number(0),
has_enclosures(false),
next(0),
previous(0),
parent(p)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	#endif
}

SingleWindow::~SingleWindow() {
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << ": " << cohorts.size() << std::endl;
	#endif

	if (cohorts.size() > 1) {
		for (uint32HashMap::iterator iter = parent->relation_map.begin() ; iter != parent->relation_map.end() ; ) {
			if (iter->second <= cohorts.back()->global_number) {
				iter = parent->relation_map.erase(iter);
			}
			else {
				++iter;
			}
		}
	}

	foreach (CohortVector, cohorts, iter, iter_end) {
		delete (*iter);
	}
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
}

void SingleWindow::appendCohort(Cohort *cohort) {
	cohort->local_number = (uint32_t)cohorts.size();
	cohort->parent = this;

	if (parent->parent->has_dep && cohort->dep_self) {
		if (cohort->dep_self <= parent->parent->dep_highest_seen || (parent->parent->dep_delimit && parent->parent->dep_highest_seen && cohort->dep_self - parent->parent->dep_highest_seen > parent->parent->dep_delimit)) {
			parent->parent->reflowDependencyWindow();
			parent->parent->gWindow->dep_map.clear();
			parent->parent->gWindow->dep_window.clear();
			parent->parent->dep_highest_seen = 0;
		}
		else {
			parent->parent->dep_highest_seen = cohort->dep_self;
		}
	}

	if (cohorts.empty()) {
		if (previous && !previous->cohorts.empty()) {
			previous->cohorts.back()->next = cohort;
			cohort->prev = previous->cohorts.back();
		}
	}
	else {
		cohort->prev = cohorts.back();
		cohorts.back()->next = cohort;
	}
	if (next && !next->cohorts.empty()) {
		next->cohorts.front()->prev = cohort;
		cohort->next = next->cohorts.front();
	}
	cohorts.push_back(cohort);
	parent->cohort_map[cohort->global_number] = cohort;
	parent->dep_window[cohort->global_number] = cohort;
	if (cohort->local_number == 0) {
		parent->cohort_map[0] = cohort;
	}
}

}
