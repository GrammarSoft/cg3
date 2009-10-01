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
#ifndef __READING_H
#define __READING_H

#include "stdafx.h"
#include "Tag.h"

namespace CG3 {
	class Cohort;

	class Reading {
	public:
		bool mapped;
		bool deleted;
		bool noprint;
		bool matched_target;
		bool matched_tests;
		uint32_t wordform;
		uint32_t baseform;
		uint32_t hash;
		uint32_t hash_plain;
		uint32_t number;
		Tag *mapping;
		Cohort *parent;
		uint32Vector hit_by;
		uint32List tags_list;
		uint32Set tags;
		uint32HashSet tags_plain;
		uint32HashSet tags_textual;
		Taguint32HashMap tags_numerical;

		Reading(Cohort *p);
		~Reading();
		void duplicateFrom(Reading &r);

		uint32_t rehash();
		static bool cmp_number(Reading *a, Reading *b);
	};

}

#endif
