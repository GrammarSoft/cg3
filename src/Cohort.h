/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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
#ifndef __COHORT_H
#define __COHORT_H

#include "stdafx.h"
#include "Reading.h"

namespace CG3 {
	class SingleWindow;
	class Reading;
	class Cohort;
	typedef std::map<uint32_t,uint32Set> RelationCtn;
	typedef std::vector<Cohort*> CohortVector;

	enum COHORT_TYPE {
		CT_ENCLOSED       = 1u,
		CT_RELATED        = 2u,
	};

	class Cohort {
	public:
		bool num_is_current;
		bool dep_done;
		uint8_t type;
		uint32_t global_number;
		uint32_t local_number;
		uint32_t wordform;
		uint32_t dep_self;
		uint32_t dep_parent;
		uint32_t is_pleft, is_pright;
		SingleWindow *parent;
		UChar *text;
		Cohort *prev, *next;
		ReadingList readings;
		ReadingList deleted;
		ReadingList delayed;
		uint32int32Map num_max, num_min;
		uint32HashSet dep_children;
		uint32HashSet possible_sets;
		CohortVector enclosed;
		RelationCtn relations;

		int32_t getMin(uint32_t key);
		int32_t getMax(uint32_t key);

		void detach();

		Cohort(SingleWindow *p);
		~Cohort();

		bool addChild(uint32_t child);
		bool remChild(uint32_t child);
		void appendReading(Reading *read);
		Reading *allocateAppendReading();
		bool addRelation(uint32_t rel, uint32_t cohort);
		bool setRelation(uint32_t rel, uint32_t cohort);
		bool remRelation(uint32_t rel, uint32_t cohort);

	private:
		void updateMinMax();
	};

	struct compare_Cohort {
		inline bool operator() (const Cohort* a, const Cohort* b) const {
			return a->global_number < b->global_number;
		}
	};

	typedef std::set<Cohort*, compare_Cohort> CohortSet;
}

#endif
