/*
* Copyright (C) 2007-2025, GrammarSoft ApS
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

#include "SingleWindow.hpp"
#include "GrammarApplicator.hpp"
#include "Window.hpp"
#include "pool.hpp"

namespace CG3 {

extern pool<SingleWindow> pool_swindows;

SingleWindow* alloc_swindow(Window* p) {
	SingleWindow* s = pool_swindows.get();
	if (s == nullptr) {
		s = new SingleWindow(p);
	}
	else {
		s->parent = p;
	}
	return s;
}

void free_swindow(SingleWindow*& s) {
	if (s == nullptr) {
		return;
	}
	pool_swindows.put(s);
	s = 0;
}

SingleWindow::SingleWindow(Window* p)
  : parent(p)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << VOIDP(this) << " " << __PRETTY_FUNCTION__ << std::endl;
	#endif
}

SingleWindow::~SingleWindow() {
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << VOIDP(this) << " " << __PRETTY_FUNCTION__ << ": " << cohorts.size() << std::endl;
	#endif

	if (cohorts.size() > 1) {
		for (auto iter = parent->relation_map.begin(); iter != parent->relation_map.end();) {
			if (iter->second <= cohorts.back()->global_number) {
				iter = parent->relation_map.erase(iter);
			}
			else {
				++iter;
			}
		}
	}

	for (auto iter : all_cohorts) {
		free_cohort(iter);
	}
	if (next && previous) {
		next->previous = previous;
		previous->next = next;
	}
	else {
		if (next) {
			next->previous = nullptr;
		}
		if (previous) {
			previous->next = nullptr;
		}
	}
}

void SingleWindow::clear() {
	if (cohorts.size() > 1) {
		for (auto iter = parent->relation_map.begin(); iter != parent->relation_map.end();) {
			if (iter->second <= cohorts.back()->global_number) {
				iter = parent->relation_map.erase(iter);
			}
			else {
				++iter;
			}
		}
	}

	for (auto iter : all_cohorts) {
		free_cohort(iter);
	}
	if (next && previous) {
		next->previous = previous;
		previous->next = next;
	}
	else {
		if (next) {
			next->previous = nullptr;
		}
		if (previous) {
			previous->next = nullptr;
		}
	}

	number = 0;
	has_enclosures = false;
	flush_after = false;
	next = nullptr;
	previous = nullptr;
	parent = nullptr;
	text.clear();
	text_post.clear();
	cohorts.clear();
	all_cohorts.clear();
	valid_rules.clear();
	hit_external.clear();
	for (auto& cs : rule_to_cohorts) {
		cs.clear();
	}
	variables_set.clear();
	variables_rem.clear();
	variables_output.clear();
	bag_of_tags.clear();
}

void SingleWindow::appendCohort(Cohort* cohort) {
	cohort->local_number = UI32(cohorts.size());
	cohort->parent = this;

	if (cohort->dep_self) {
		parent->parent->dep_highest_seen = cohort->dep_self;
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
	all_cohorts.push_back(cohort);
	parent->cohort_map[cohort->global_number] = cohort;
	parent->dep_window[cohort->global_number] = cohort;
	if (cohort->local_number == 0) {
		parent->cohort_map[0] = cohort;
	}
}
}
