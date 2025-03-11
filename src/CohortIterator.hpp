/*
* Copyright (C) 2007-2025, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_COHORTITERATOR_H
#define c6d28b7452ec699b_COHORTITERATOR_H

#include "stdafx.hpp"
#include "Cohort.hpp"
#include "SingleWindow.hpp"

namespace CG3 {
class ContextualTest;

class CohortIterator {
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = Cohort*;
	using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using reference = value_type&;

	CohortIterator(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	virtual ~CohortIterator();

	bool operator==(const CohortIterator& other) const;
	bool operator!=(const CohortIterator& other) const;

	virtual CohortIterator& operator++();

	Cohort* operator*();

	virtual void reset(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

protected:
	bool m_span = false;
	Cohort* m_cohort = nullptr;
	const ContextualTest* m_test = nullptr;
};

class TopologyLeftIter : public CohortIterator {
public:
	TopologyLeftIter(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	TopologyLeftIter& operator++();
};

class TopologyRightIter : public CohortIterator {
public:
	TopologyRightIter(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	TopologyRightIter& operator++();
};

class DepParentIter : public CohortIterator {
public:
	DepParentIter(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	DepParentIter& operator++();

	void reset(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

protected:
	CohortSet m_seen;
};

class DepDescendentIter : public CohortIterator {
public:
	DepDescendentIter(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	DepDescendentIter& operator++();

	void reset(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

protected:
	CohortSet m_descendents;
	CohortSet::const_iterator m_ai;
};

class DepAncestorIter : public CohortIterator {
public:
	DepAncestorIter(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	DepAncestorIter& operator++();

	void reset(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

protected:
	CohortSet m_ancestors;
	CohortSet::const_iterator m_ai;
};

class CohortSetIter : public CohortIterator {
public:
	CohortSetIter(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	void addCohort(Cohort* cohort);

	CohortSetIter& operator++();

protected:
	Cohort* m_origcohort;
	CohortSet m_cohortset;
	CohortSet::const_iterator m_cohortsetiter;
};

class MultiCohortIterator {
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = Cohort*;
	using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using reference = value_type&;

	MultiCohortIterator(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	virtual ~MultiCohortIterator();

	bool operator==(const MultiCohortIterator& other) const;
	bool operator!=(const MultiCohortIterator& other) const;

	virtual MultiCohortIterator& operator++();

	CohortIterator* operator*();

protected:
	bool m_span;
	Cohort* m_cohort;
	const ContextualTest* m_test;
	CohortSet m_seen;
	std::unique_ptr<CohortSetIter> m_cohortiter;
};

class ChildrenIterator : public MultiCohortIterator {
public:
	ChildrenIterator(Cohort* cohort = nullptr, const ContextualTest* test = nullptr, bool span = false);

	ChildrenIterator& operator++();

protected:
	uint32_t m_depth;
};
}

#endif
