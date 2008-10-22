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

#ifndef __SINGLEWINDOW_H
#define __SINGLEWINDOW_H

#include "stdafx.h"
#include "Window.h"
#include "Cohort.h"

namespace CG3 {

	class SingleWindow {
	public:
		uint32_t number;
		SingleWindow *next, *previous;
		Window *parent;

		std::vector<Cohort*> cohorts;
		UChar *text;
		uint32_t hash, hash_tags, hash_mapped, hash_plain, hash_textual;
		uint32Set valid_rules;
		uint32Setuint32HashMap rule_to_cohorts;

		SingleWindow(Window *p);
		~SingleWindow();

		void appendCohort(Cohort *cohort);
	};

}

#endif
