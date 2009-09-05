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

#pragma once
#ifndef __SINGLEWINDOW_H
#define __SINGLEWINDOW_H

#include "stdafx.h"
#include "Cohort.h"
#include "Rule.h"

namespace CG3 {

	class SingleWindow {
	public:
		uint32_t number;
		SingleWindow *next, *previous;
		Window *parent;
		UChar *text;
		std::vector<Cohort*> cohorts;
		uint32Set valid_rules;
		RuleToCohortsMap rule_to_cohorts;

		SingleWindow(Window *p);
		~SingleWindow();

		void appendCohort(Cohort *cohort);
	};

}

#endif
