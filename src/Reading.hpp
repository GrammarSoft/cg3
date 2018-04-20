/*
* Copyright (C) 2007-2018, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_READING_H
#define c6d28b7452ec699b_READING_H

#include "stdafx.hpp"
#include "Tag.hpp"
#include "sorted_vector.hpp"
#include "bloomish.hpp"

namespace CG3 {
class Cohort;
class Reading;

typedef std::vector<Reading*> ReadingList;

class Reading {
public:
	uint8_t mapped : 1;
	uint8_t deleted : 1;
	uint8_t noprint : 1;
	uint8_t matched_target : 1;
	uint8_t matched_tests : 1;
	uint8_t immutable : 1;

	uint32_t baseform;
	uint32_t hash;
	uint32_t hash_plain;
	uint32_t number;
	uint32Bloomish tags_bloom;
	uint32Bloomish tags_plain_bloom;
	uint32Bloomish tags_textual_bloom;
	Tag* mapping;
	Cohort* parent;
	Reading* next;
	uint32Vector hit_by;
	typedef uint32Vector tags_list_t;
	tags_list_t tags_list;
	uint32SortedVector tags;
	uint32SortedVector tags_plain;
	uint32SortedVector tags_textual;
	typedef bc::flat_map<uint32_t, Tag*> tags_numerical_t;
	tags_numerical_t tags_numerical;

	// ToDo: Remove for real ordered mode
	UString tags_string;
	uint32_t tags_string_hash = 0;

	Reading(Cohort* p = 0);
	Reading(const Reading& r);
	~Reading();
	void clear();

	Reading* allocateReading(Cohort* p);
	Reading* allocateReading(const Reading& r);

	uint32_t rehash();
	static bool cmp_number(Reading* a, Reading* b);
};

Reading* alloc_reading(Cohort* p = 0);
Reading* alloc_reading(const Reading& r);
void free_reading(Reading* r);
}

#endif
