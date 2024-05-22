/*
* Copyright (C) 2007-2024, GrammarSoft ApS
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

#pragma once
#ifndef c6d28b7452ec699b_SINGLEWINDOW_H
#define c6d28b7452ec699b_SINGLEWINDOW_H

#include "stdafx.hpp"
#include "Cohort.hpp"
#include "Rule.hpp"
#include "interval_vector.hpp"
#include "sorted_vector.hpp"

namespace CG3 {
class Window;

class SingleWindow {
public:
	uint32_t number = 0;
	bool has_enclosures = false;
	bool flush_after = false;
	SingleWindow *next = nullptr, *previous = nullptr;
	Window* parent = nullptr;
	UString text;
	UString text_post;
	CohortVector all_cohorts;
	CohortVector cohorts;
	uint32IntervalVector valid_rules;
	uint32SortedVector hit_external;
	std::vector<CohortSet> rule_to_cohorts;
	// Used by GrammarApplicator::runSingleRule so that it doesn't need to allocate a new one or edit a rule's actual list when applying a subrule of WITH
	std::unique_ptr<CohortSet> nested_rule_to_cohorts;
	uint32FlatHashMap variables_set;
	uint32FlatHashSet variables_rem;
	uint32SortedVector variables_output;
	Reading bag_of_tags;

	SingleWindow(Window* p);
	~SingleWindow();
	void clear();

	void appendCohort(Cohort* cohort);
};

SingleWindow* alloc_swindow(Window* p);
void free_swindow(SingleWindow*& s);

inline bool less_Cohort(const Cohort* a, const Cohort* b) {
	if (a->local_number == b->local_number) {
		return a->parent->number < b->parent->number;
	}
	return a->local_number < b->local_number;
}

struct compare_Cohort {
	bool operator()(const Cohort* a, const Cohort* b) const {
		return less_Cohort(a, b);
	}
};
}

#endif
