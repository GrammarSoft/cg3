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

#include "CohortIterator.h"
#include "ContextualTest.h"
#include "Window.h"
#include "SingleWindow.h"

namespace CG3 {

CohortIterator::CohortIterator(Cohort *cohort, const ContextualTest *test, bool span) :
m_span(span),
m_cohort(cohort),
m_test(test)
{
}

CohortIterator::~CohortIterator() {
}

bool CohortIterator::operator==(const CohortIterator& other) {
	return (m_cohort == other.m_cohort);
}
bool CohortIterator::operator!=(const CohortIterator& other) {
	return (m_cohort != other.m_cohort);
}

CohortIterator& CohortIterator::operator++() {
	m_cohort = 0;
	return *this;
}

Cohort* CohortIterator::operator*() {
	return m_cohort;
}

TopologyLeftIter::TopologyLeftIter(Cohort *cohort, const ContextualTest *test, bool span) :
CohortIterator(cohort, test, span)
{
}

TopologyLeftIter& TopologyLeftIter::operator++() {
	if (m_cohort->prev && m_cohort->prev->parent != m_cohort->parent && !(m_test->pos & (POS_SPAN_BOTH|POS_SPAN_LEFT) || m_span)) {
		m_cohort = 0;
	}
	else {
		do {
			m_cohort = m_cohort->prev;
		} while (m_cohort && m_cohort->is_enclosed);
	}
	return *this;
}

TopologyRightIter::TopologyRightIter(Cohort *cohort, const ContextualTest *test, bool span) :
CohortIterator(cohort, test, span)
{
}

TopologyRightIter& TopologyRightIter::operator++() {
	if (m_cohort->next && m_cohort->next->parent != m_cohort->parent && !(m_test->pos & (POS_SPAN_BOTH|POS_SPAN_RIGHT) || m_span)) {
		m_cohort = 0;
	}
	else {
		do {
			m_cohort = m_cohort->next;
		} while (m_cohort && m_cohort->is_enclosed);
	}
	return *this;
}

DepParentIter::DepParentIter(Cohort *cohort, const ContextualTest *test, bool span) :
CohortIterator(cohort, test, span)
{
	++(*this);
}

DepParentIter& DepParentIter::operator++() {
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

CohortSetIter::CohortSetIter(Cohort *cohort, const ContextualTest *test, bool span) :
CohortIterator(cohort, test, span),
m_origcohort(cohort),
m_cohortsetiter(m_cohortset.end())
{
}

void CohortSetIter::addCohort(Cohort *cohort) {
	m_cohortset.insert(cohort);
	m_cohortsetiter = m_cohortset.begin();
}

CohortSetIter& CohortSetIter::operator++() {
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

MultiCohortIterator::MultiCohortIterator(Cohort *cohort, const ContextualTest *test, bool span) :
m_span(span),
m_cohort(cohort),
m_test(test),
m_cohortiter(0)
{
}

MultiCohortIterator::~MultiCohortIterator() {
			delete m_cohortiter;
}

bool MultiCohortIterator::operator==(const MultiCohortIterator& other) {
	return (m_cohort == other.m_cohort);
}
bool MultiCohortIterator::operator!=(const MultiCohortIterator& other) {
	return (m_cohort != other.m_cohort);
}

MultiCohortIterator& MultiCohortIterator::operator++() {
	m_cohort = 0;
	return *this;
}

CohortIterator* MultiCohortIterator::operator*() {
	return m_cohortiter;
}

// ToDo: Iterative deepening depth-first search
ChildrenIterator::ChildrenIterator(Cohort *cohort, const ContextualTest *test, bool span) :
MultiCohortIterator(cohort, test, span),
m_depth(0)
{
}

ChildrenIterator& ChildrenIterator::operator++() {
	delete m_cohortiter;
	m_cohortiter = 0;
	++m_depth;
	uint32HashSet *top = &(m_cohort->dep_children);
	if (!top->empty()) {
		m_cohortiter = new CohortSetIter(m_cohort, m_test, m_span);
	}
	return *this;
}

}
