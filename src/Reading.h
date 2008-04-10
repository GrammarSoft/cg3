/*
* Copyright (C) 2007, GrammarSoft Aps
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

#ifndef __READING_H
#define __READING_H

#include "stdafx.h"

namespace CG3 {

	class Reading {
	public:
		Cohort *parent;

		uint32_t wordform;
		uint32_t baseform;
		uint32_t hash;
		bool mapped;
		bool deleted;
		uint32Vector hit_by;
		bool noprint;
		uint32List tags_list;
		uint32Set tags;
		// ToDo: Get these back to normal ones; no need to recycle elements of a recycled object
		uint32HashSet *tags_plain;
		uint32HashSet *tags_mapped;
		uint32HashSet *tags_textual;
		uint32HashSet *tags_numerical;

		uint32HashSet possible_sets;

		UChar *text;

		bool matched_target;
		bool matched_tests;
		uint32_t current_mapping_tag;

		Reading(Cohort *p);
		~Reading();
		void clear(Cohort *p);

		uint32_t rehash();
	};

}

#endif
