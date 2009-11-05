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
#ifndef __COHORTITERATOR_H
#define __COHORTITERATOR_H

#include "stdafx.h"
#include "Cohort.h"
#include "ContextualTest.h"

namespace CG3 {
	class CohortIterator : public std::iterator<std::input_iterator_tag, Cohort*> {
	public:
		CohortIterator(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false) :
		m_span(span),
		m_cohort(cohort),
		m_test(test)
		{
		}

		virtual ~CohortIterator() {
		}

		bool operator ==(const CohortIterator& other) {
			return (m_cohort == other.m_cohort);
		}
		bool operator !=(const CohortIterator& other) {
			return (m_cohort != other.m_cohort);
		}

		virtual CohortIterator& operator++() {
			m_cohort = 0;
			return *this;
		}

		Cohort* operator*() {
			return m_cohort;
		}

	protected:
		bool m_span;
		Cohort *m_cohort;
		const ContextualTest *m_test;
	};

	class TopologyLeftIter : public CohortIterator {
	public:
		TopologyLeftIter(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false) :
		CohortIterator(cohort, test, span)
		{
		}

		TopologyLeftIter& operator++() {
			if (m_cohort->prev && m_cohort->prev->parent != m_cohort->parent && !(m_test->pos & (POS_SPAN_BOTH|POS_SPAN_LEFT) || m_span)) {
				m_cohort = 0;
			}
			else {
				m_cohort = m_cohort->prev;
			}
			return *this;
		}
	};

	class TopologyRightIter : public CohortIterator {
	public:
		TopologyRightIter(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false) :
		CohortIterator(cohort, test, span)
		{
		}

		TopologyRightIter& operator++() {
			if (m_cohort->next && m_cohort->next->parent != m_cohort->parent && !(m_test->pos & (POS_SPAN_BOTH|POS_SPAN_RIGHT) || m_span)) {
				m_cohort = 0;
			}
			else {
				m_cohort = m_cohort->next;
			}
			return *this;
		}
	};

	class DepParentIter : public CohortIterator {
	public:
		DepParentIter(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false) :
		CohortIterator(cohort, test, span)
		{
			++(*this);
		}

		DepParentIter& operator++() {
			if (m_cohort->dep_parent != std::numeric_limits<uint32_t>::max()) {
				std::map<uint32_t,Cohort*>::iterator it = m_cohort->parent->parent->cohort_map.find(m_cohort->dep_parent);
				if (it != m_cohort->parent->parent->cohort_map.end()) {
					Cohort *cohort = it->second;
					if (m_seen.find(cohort) == m_seen.end()) {
						m_seen.insert(m_cohort);
						if (cohort->parent == m_cohort->parent || (m_test->pos & POS_SPAN_BOTH) || m_span) {
							m_cohort = cohort;
						}
						else if (cohort->parent->number < m_cohort->parent->number && (m_test->pos & POS_SPAN_LEFT)) {
							m_cohort = cohort;
						}
						else if (cohort->parent->number > m_cohort->parent->number && (m_test->pos & POS_SPAN_RIGHT)) {
							m_cohort = cohort;
						}
						else {
							m_cohort = 0;
						}
						return *this;
					}
				}
			}
			m_cohort = 0;
			return *this;
		}

	protected:
		CohortSet m_seen;
	};

	class CohortSetIter : public CohortIterator {
	public:
		CohortSetIter(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false) :
		CohortIterator(cohort, test, span),
		m_origcohort(cohort),
		m_cohortsetiter(m_cohortset.end())
		{
		}

		void addCohort(Cohort *cohort) {
			m_cohortset.insert(cohort);
			m_cohortsetiter = m_cohortset.begin();
		}

		CohortSetIter& operator++() {
			m_cohort = 0;
			for (; m_cohortsetiter != m_cohortset.end() ; ++m_cohortsetiter) {
				Cohort *cohort = *m_cohortsetiter;
				if (cohort->parent == m_origcohort->parent || (m_test->pos & POS_SPAN_BOTH) || m_span) {
					m_cohort = cohort;
					break;
				}
				else if (cohort->parent->number < m_origcohort->parent->number && (m_test->pos & POS_SPAN_LEFT)) {
					m_cohort = cohort;
					break;
				}
				else if (cohort->parent->number > m_origcohort->parent->number && (m_test->pos & POS_SPAN_RIGHT)) {
					m_cohort = cohort;
					break;
				}
			}
			return *this;
		}

	protected:
		Cohort *m_origcohort;
		CohortSet m_cohortset;
		CohortSet::iterator m_cohortsetiter;
	};

	class MultiCohortIterator : public std::iterator<std::input_iterator_tag, Cohort*> {
	public:
		MultiCohortIterator(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false) :
		m_span(span),
		m_cohort(cohort),
		m_test(test),
		m_cohortiter(0)
		{
		}

		virtual ~MultiCohortIterator() {
			delete m_cohortiter;
		}

		bool operator ==(const MultiCohortIterator& other) {
			return (m_cohort == other.m_cohort);
		}
		bool operator !=(const MultiCohortIterator& other) {
			return (m_cohort != other.m_cohort);
		}

		virtual MultiCohortIterator& operator++() {
			m_cohort = 0;
			return *this;
		}

		CohortIterator* operator*() {
			return m_cohortiter;
		}

	protected:
		bool m_span;
		Cohort *m_cohort;
		const ContextualTest *m_test;
		CohortSet m_seen;
		CohortSetIter *m_cohortiter;
	};

	// ToDo: Iterative deepening depth-first search
	class ChildrenIterator : public MultiCohortIterator {
	public:
		ChildrenIterator(Cohort *cohort = 0, const ContextualTest *test = 0, bool span = false) :
		MultiCohortIterator(cohort, test, span),
		m_depth(0)
		{
		}

		ChildrenIterator& operator++() {
			delete m_cohortiter;
			m_cohortiter = 0;
			++m_depth;
			uint32HashSet *top = &(m_cohort->dep_children);
			if (!top->empty()) {
				m_cohortiter = new CohortSetIter(m_cohort, m_test, m_span);
			}
			return *this;
		}

	protected:
		uint32_t m_depth;
	};
}

#endif
