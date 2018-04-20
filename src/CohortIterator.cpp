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

#include "CohortIterator.hpp"
#include "ContextualTest.hpp"
#include "Window.hpp"

namespace CG3 {

CohortIterator::CohortIterator(Cohort* cohort, const ContextualTest* test, bool span)
  : m_span(span)
  , m_cohort(cohort)
  , m_test(test)
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

void CohortIterator::reset(Cohort* cohort, const ContextualTest* test, bool span) {
	m_span = span;
	m_cohort = cohort;
	m_test = test;
}

TopologyLeftIter::TopologyLeftIter(Cohort* cohort, const ContextualTest* test, bool span)
  : CohortIterator(cohort, test, span)
{
}

TopologyLeftIter& TopologyLeftIter::operator++() {
	if (!m_cohort || !m_test) {
		return *this;
	}
	if (m_cohort->prev && m_cohort->prev->parent != m_cohort->parent && !(m_test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT) || m_span)) {
		m_cohort = 0;
	}
	else {
		do {
			m_cohort = m_cohort->prev;
		} while (m_cohort && (m_cohort->type & CT_ENCLOSED));
	}
	return *this;
}

TopologyRightIter::TopologyRightIter(Cohort* cohort, const ContextualTest* test, bool span)
  : CohortIterator(cohort, test, span)
{
}

TopologyRightIter& TopologyRightIter::operator++() {
	if (!m_cohort || !m_test) {
		return *this;
	}
	if (m_cohort->next && m_cohort->next->parent != m_cohort->parent && !(m_test->pos & (POS_SPAN_BOTH | POS_SPAN_RIGHT) || m_span)) {
		m_cohort = 0;
	}
	else {
		do {
			m_cohort = m_cohort->next;
		} while (m_cohort && (m_cohort->type & CT_ENCLOSED));
	}
	return *this;
}

DepParentIter::DepParentIter(Cohort* cohort, const ContextualTest* test, bool span)
  : CohortIterator(cohort, test, span)
{
	++(*this);
}

DepParentIter& DepParentIter::operator++() {
	if (!m_cohort || !m_test) {
		return *this;
	}
	if (m_cohort->dep_parent != DEP_NO_PARENT) {
		std::map<uint32_t, Cohort*>::iterator it = m_cohort->parent->parent->cohort_map.find(m_cohort->dep_parent);
		if (it != m_cohort->parent->parent->cohort_map.end()) {
			Cohort* cohort = it->second;
			if (cohort->type & CT_REMOVED) {
				m_cohort = 0;
				return *this;
			}
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

void DepParentIter::reset(Cohort* cohort, const ContextualTest* test, bool span) {
	CohortIterator::reset(cohort, test, span);
	m_seen.clear();
	++(*this);
}

DepDescendentIter::DepDescendentIter(Cohort* cohort, const ContextualTest* test, bool span)
  : CohortIterator(cohort, test, span)
{
	reset(cohort, test, span);
}

DepDescendentIter& DepDescendentIter::operator++() {
	++m_ai;
	m_cohort = 0;
	if (m_ai != m_descendents.end()) {
		m_cohort = *m_ai;
	}
	return *this;
}

void DepDescendentIter::reset(Cohort* cohort, const ContextualTest* test, bool span) {
	CohortIterator::reset(cohort, test, span);
	m_descendents.clear();
	m_cohort = 0;

	if (cohort && test) {
		for (auto dter : cohort->dep_children) {
			if (cohort->parent->parent->cohort_map.find(dter) == cohort->parent->parent->cohort_map.end()) {
				continue;
			}
			Cohort* current = cohort->parent->parent->cohort_map.find(dter)->second;
			bool good = true;
			if (current->parent != cohort->parent) {
				if ((!(test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT))) && current->parent->number < cohort->parent->number) {
					good = false;
				}
				else if ((!(test->pos & (POS_SPAN_BOTH | POS_SPAN_RIGHT))) && current->parent->number > cohort->parent->number) {
					good = false;
				}
			}
			if (good) {
				m_descendents.insert(current);
			}
		}

		CohortSet m_seen;
		m_seen.insert(cohort);

		bool added = false;
		do {
			added = false;
			CohortSet to_add;

			for (auto cohort_inner : m_descendents) {
				if (m_seen.find(cohort_inner) != m_seen.end()) {
					continue;
				}
				m_seen.insert(cohort_inner);

				for (auto dter : cohort_inner->dep_children) {
					if (cohort_inner->parent->parent->cohort_map.find(dter) == cohort_inner->parent->parent->cohort_map.end()) {
						continue;
					}
					Cohort* current = cohort_inner->parent->parent->cohort_map.find(dter)->second;
					bool good = true;
					if (current->parent != cohort->parent) {
						if ((!(test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT))) && current->parent->number < cohort->parent->number) {
							good = false;
						}
						else if ((!(test->pos & (POS_SPAN_BOTH | POS_SPAN_RIGHT))) && current->parent->number > cohort->parent->number) {
							good = false;
						}
					}
					if (good) {
						to_add.insert(current);
						added = true;
					}
				}
			}

			for (auto iter : to_add) {
				m_descendents.insert(iter);
			}
		} while (added);

		if (test->pos & POS_LEFT) {
			m_seen.assign(m_descendents.begin(), m_descendents.lower_bound(cohort));
			m_descendents.swap(m_seen);
		}
		if (test->pos & POS_RIGHT) {
			m_seen.assign(m_descendents.lower_bound(cohort), m_descendents.end());
			m_descendents.swap(m_seen);
		}
		if (test->pos & POS_SELF) {
			m_descendents.insert(cohort);
		}
		if ((test->pos & POS_RIGHTMOST) && !m_descendents.empty()) {
			CohortSet::container& cont = m_descendents.get();
			std::reverse(cont.begin(), cont.end());
		}
	}

	m_ai = m_descendents.begin();
	if (m_ai != m_descendents.end()) {
		m_cohort = *m_ai;
	}
}

DepAncestorIter::DepAncestorIter(Cohort* cohort, const ContextualTest* test, bool span)
  : CohortIterator(cohort, test, span)
{
	reset(cohort, test, span);
}

DepAncestorIter& DepAncestorIter::operator++() {
	++m_ai;
	m_cohort = 0;
	if (m_ai != m_ancestors.end()) {
		m_cohort = *m_ai;
	}
	return *this;
}

void DepAncestorIter::reset(Cohort* cohort, const ContextualTest* test, bool span) {
	CohortIterator::reset(cohort, test, span);
	m_ancestors.clear();
	m_cohort = 0;

	if (cohort && test) {
		for (Cohort* current = cohort; current;) {
			if (cohort->parent->parent->cohort_map.find(current->dep_parent) == cohort->parent->parent->cohort_map.end()) {
				break;
			}
			current = cohort->parent->parent->cohort_map.find(current->dep_parent)->second;
			bool good = true;
			if (current->parent != cohort->parent) {
				if ((!(test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT))) && current->parent->number < cohort->parent->number) {
					good = false;
				}
				else if ((!(test->pos & (POS_SPAN_BOTH | POS_SPAN_RIGHT))) && current->parent->number > cohort->parent->number) {
					good = false;
				}
			}
			if (good) {
				// If insertion fails, we've come around in a loop, so don't continue looping
				if (!m_ancestors.insert(current).second) {
					break;
				}
			}
		}

		if (test->pos & POS_LEFT) {
			CohortSet tmp;
			tmp.assign(m_ancestors.begin(), m_ancestors.lower_bound(cohort));
			m_ancestors.swap(tmp);
		}
		if (test->pos & POS_RIGHT) {
			CohortSet tmp;
			tmp.assign(m_ancestors.lower_bound(cohort), m_ancestors.end());
			m_ancestors.swap(tmp);
		}
		if (test->pos & POS_SELF) {
			m_ancestors.insert(cohort);
		}
		if ((test->pos & POS_RIGHTMOST) && !m_ancestors.empty()) {
			CohortSet::container& cont = m_ancestors.get();
			std::reverse(cont.begin(), cont.end());
		}
	}

	m_ai = m_ancestors.begin();
	if (m_ai != m_ancestors.end()) {
		m_cohort = *m_ai;
	}
}

CohortSetIter::CohortSetIter(Cohort* cohort, const ContextualTest* test, bool span)
  : CohortIterator(cohort, test, span)
  , m_origcohort(cohort)
  , m_cohortsetiter(m_cohortset.end())
{
}

void CohortSetIter::addCohort(Cohort* cohort) {
	m_cohortset.insert(cohort);
	m_cohortsetiter = m_cohortset.begin();
}

CohortSetIter& CohortSetIter::operator++() {
	m_cohort = 0;
	for (; m_cohortsetiter != m_cohortset.end(); ++m_cohortsetiter) {
		Cohort* cohort = *m_cohortsetiter;
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

MultiCohortIterator::MultiCohortIterator(Cohort* cohort, const ContextualTest* test, bool span)
  : m_span(span)
  , m_cohort(cohort)
  , m_test(test)
{
}

MultiCohortIterator::~MultiCohortIterator() {
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
	return m_cohortiter.get();
}

// ToDo: Iterative deepening depth-first search
ChildrenIterator::ChildrenIterator(Cohort* cohort, const ContextualTest* test, bool span)
  : MultiCohortIterator(cohort, test, span)
  , m_depth(0)
{
}

ChildrenIterator& ChildrenIterator::operator++() {
	m_cohortiter.reset();
	++m_depth;
	if (!m_cohort->dep_children.empty()) {
		m_cohortiter.reset(new CohortSetIter(m_cohort, m_test, m_span));
	}
	return *this;
}
}
