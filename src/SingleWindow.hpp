/*
* Copyright (C) 2007-2014, GrammarSoft ApS
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
		uint32_t number;
		bool has_enclosures;
		SingleWindow *next, *previous;
		Window *parent;
		UString text;
		CohortVector cohorts;
		uint32IntervalVector valid_rules;
		uint32SortedVector hit_external;
		uint32ToCohortsMap rule_to_cohorts;
		uint32FlatHashMap variables_set;
		uint32FlatHashSet variables_rem;
		uint32SortedVector variables_output;

		SingleWindow(Window *p);
		~SingleWindow();

		void appendCohort(Cohort *cohort);
	};

}

#endif
