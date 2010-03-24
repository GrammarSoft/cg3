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
#ifndef __COHORTITERATOR_H
#define __COHORTITERATOR_H

#include "stdafx.h"
#include "Cohort.h"

namespace CG3 {
	class ContextualTest;

	class CohortIterator : public std::iterator<std::input_iterator_tag, Cohort*> {
	public:
		CohortIterator(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

		virtual ~CohortIterator();

		bool operator==(const CohortIterator& other);
		bool operator!=(const CohortIterator& other);

		virtual CohortIterator& operator++();

		Cohort* operator*();

		virtual void reset(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

	protected:
		bool m_span;
		Cohort *m_cohort;
		const ContextualTest *m_test;
	};

	class TopologyLeftIter : public CohortIterator {
	public:
		TopologyLeftIter(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

		TopologyLeftIter& operator++();
	};

	class TopologyRightIter : public CohortIterator {
	public:
		TopologyRightIter(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

		TopologyRightIter& operator++();
	};

	class DepParentIter : public CohortIterator {
	public:
		DepParentIter(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

		DepParentIter& operator++();

		void reset(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

	protected:
		CohortSet m_seen;
	};

	class CohortSetIter : public CohortIterator {
	public:
		CohortSetIter(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

		void addCohort(Cohort *cohort);

		CohortSetIter& operator++();

	protected:
		Cohort *m_origcohort;
		CohortSet m_cohortset;
		CohortSet::iterator m_cohortsetiter;
	};

	class MultiCohortIterator : public std::iterator<std::input_iterator_tag, Cohort*> {
	public:
		MultiCohortIterator(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

		virtual ~MultiCohortIterator();

		bool operator==(const MultiCohortIterator& other);
		bool operator!=(const MultiCohortIterator& other);

		virtual MultiCohortIterator& operator++();

		CohortIterator* operator*();

	protected:
		bool m_span;
		Cohort *m_cohort;
		const ContextualTest *m_test;
		CohortSet m_seen;
		CohortSetIter *m_cohortiter;
	};

	class ChildrenIterator : public MultiCohortIterator {
	public:
		ChildrenIterator(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false);

		ChildrenIterator& operator++();

	protected:
		uint32_t m_depth;
	};
}

#endif
