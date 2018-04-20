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
#ifndef c6d28b7452ec699b_COHORT_H
#define c6d28b7452ec699b_COHORT_H

#include "stdafx.hpp"
#include "Reading.hpp"
#include "sorted_vector.hpp"
#include "flat_unordered_set.hpp"

namespace CG3 {
class SingleWindow;
class Reading;
class Cohort;
typedef bc::flat_map<uint32_t, uint32SortedVector> RelationCtn;
typedef std::vector<Cohort*> CohortVector;

enum {
	CT_ENCLOSED    = (1 <<  0),
	CT_RELATED     = (1 <<  1),
	CT_REMOVED     = (1 <<  2),
	CT_NUM_CURRENT = (1 <<  3),
	CT_DEP_DONE    = (1 <<  4),
};

// ToDo: Would love to make this a constexpr global, but that's C++11
#define DEP_NO_PARENT std::numeric_limits<uint32_t>::max()

class Cohort {
public:
	uint8_t type;
	// ToDo: Get rid of global_number in favour of Cohort* relations
	uint32_t global_number;
	uint32_t local_number;
	Tag* wordform;
	uint32_t dep_self;
	uint32_t dep_parent;
	uint32_t is_pleft, is_pright;
	SingleWindow* parent;
	UString text;
	Cohort *prev, *next;
	Reading* wread;
	ReadingList readings;
	ReadingList deleted;
	ReadingList delayed;
	typedef bc::flat_map<uint32_t, double> num_t;
	num_t num_max, num_min;
	uint32SortedVector dep_children;
	boost::dynamic_bitset<> possible_sets;
	CohortVector enclosed;
	CohortVector removed;
	RelationCtn relations;
	RelationCtn relations_input;

	double getMin(uint32_t key);
	double getMax(uint32_t key);

	void detach();

	Cohort(SingleWindow* p);
	~Cohort();
	void clear();

	void addChild(uint32_t child);
	void remChild(uint32_t child);
	void appendReading(Reading* read);
	Reading* allocateAppendReading();
	Reading* allocateAppendReading(Reading& r);
	bool addRelation(uint32_t rel, uint32_t cohort);
	bool setRelation(uint32_t rel, uint32_t cohort);
	bool remRelation(uint32_t rel, uint32_t cohort);

private:
	void updateMinMax();
};

struct compare_Cohort;

typedef sorted_vector<Cohort*, compare_Cohort> CohortSet;
typedef std::unordered_map<uint32_t, CohortSet> uint32ToCohortsMap;

Cohort* alloc_cohort(SingleWindow* p);
void free_cohort(Cohort* c);
}

#endif
