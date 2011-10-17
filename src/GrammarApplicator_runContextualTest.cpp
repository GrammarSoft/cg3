/*
* Copyright (C) 2007-2011, GrammarSoft ApS
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

#include "GrammarApplicator.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"
#include "ContextualTest.h"

namespace CG3 {

Cohort *GrammarApplicator::runSingleTest(Cohort *cohort, const ContextualTest *test, bool *brk, bool *retval, Cohort **deep, Cohort *origin) {
	if (test->pos & POS_MARK_SET) {
		mark = cohort;
	}
	if (test->pos & POS_ATTACH_TO) {
		attach_to = cohort;
	}
	if (deep) {
		*deep = cohort;
	}
	if (test->pos & POS_CAREFUL) {
		*retval = doesSetMatchCohortCareful(*cohort, test->target, test, test->pos);
	}
	bool foundfirst = *retval;
	if (!foundfirst || !(test->pos & POS_CAREFUL)) {
		foundfirst = doesSetMatchCohortNormal(*cohort, test->target, test, test->pos);
		if (!(test->pos & POS_CAREFUL)) {
			*retval = foundfirst;
		}
	}
	if (origin && (test->offset != 0 || (test->pos & (POS_SCANALL|POS_SCANFIRST))) && origin == cohort && origin->local_number != 0) {
		*retval = false;
		*brk = true;
	}
	if (test->pos & POS_NOT) {
		*retval = !*retval;
	}
	if (*retval && test->linked) {
		if (test->linked->pos & POS_NO_PASS_ORIGIN) {
			*retval = (runContextualTest(cohort->parent, cohort->local_number, test->linked, deep, cohort) != 0);
		}
		else {
			*retval = (runContextualTest(cohort->parent, cohort->local_number, test->linked, deep, origin) != 0);
		}
	}
	if (foundfirst && (test->pos & POS_SCANFIRST)) {
		*brk = true;
	}
	else if (!(test->pos & (POS_SCANALL|POS_SCANFIRST))) {
		*brk = true;
	}
	if (test->barrier) {
		bool barrier = doesSetMatchCohortNormal(*cohort, test->barrier, test, test->pos & ~POS_CAREFUL);
		if (barrier) {
			*brk = true;
		}
	}
	if (test->cbarrier) {
		bool cbarrier = doesSetMatchCohortCareful(*cohort, test->cbarrier, test, test->pos | POS_CAREFUL);
		if (cbarrier) {
			*brk = true;
		}
	}
	if (foundfirst && *retval) {
		*brk = true;
	}
	return cohort;
}

Cohort *GrammarApplicator::runSingleTest(SingleWindow *sWindow, size_t i, const ContextualTest *test, bool *brk, bool *retval, Cohort **deep, Cohort *origin) {
	if (i >= sWindow->cohorts.size()) {
		*brk = true;
		*retval = false;
		return 0;
	}
	Cohort *cohort = sWindow->cohorts[i];
	return runSingleTest(cohort, test, brk, retval, deep, origin);
}

Cohort *getCohortInWindow(SingleWindow *& sWindow, size_t position, const ContextualTest *test, int32_t& pos) {
	Cohort *cohort = 0;
	pos = static_cast<int32_t>(position) + test->offset;
	// ToDo: (NOT *) and (*C) tests can be cached
	if (test->pos & POS_ABSOLUTE) {
		if (test->offset < 0) {
			pos = (static_cast<int32_t>(sWindow->cohorts.size())-1) - test->offset;
		}
		else {
			pos = test->offset;
		}
	}
	if (pos >= 0) {
		if (pos >= static_cast<int32_t>(sWindow->cohorts.size()) && (test->pos & (POS_SPAN_RIGHT|POS_SPAN_BOTH)) && sWindow->next) {
			sWindow = sWindow->next;
			pos = 0;
		}
	}
	else {
		if ((test->pos & (POS_SPAN_LEFT|POS_SPAN_BOTH)) && sWindow->previous) {
			sWindow = sWindow->previous;
			pos = static_cast<int32_t>(sWindow->cohorts.size())-1;
		}
	}
	if (pos >= 0 && pos < static_cast<int32_t>(sWindow->cohorts.size())) {
		cohort = sWindow->cohorts[pos];
	}
	return cohort;
}

Cohort *GrammarApplicator::runContextualTest(SingleWindow *sWindow, size_t position, const ContextualTest *test, Cohort **deep, Cohort *origin) {
	if (test->pos & POS_UNKNOWN) {
		u_fprintf(ux_stderr, "Error: Contextual tests with position '?' cannot be used directly. Provide an override position.\n");
		CG3Quit(1);
	}

	Cohort *cohort = 0;
	bool retval = true;

	ticks tstamp(gtimer);
	if (statistics) {
		tstamp = getticks();
	}

	if (test->pos & POS_MARK_JUMP) {
		sWindow = mark->parent;
		position = mark->local_number;
	}
	int32_t pos = static_cast<int32_t>(position) + test->offset;

	if (test->tmpl) {
		uint32_t orgpos = test->tmpl->pos;
		int32_t orgoffset = test->tmpl->offset;
		uint32_t orgcbar = test->tmpl->cbarrier;
		uint32_t orgbar = test->tmpl->barrier;
		if (test->pos & POS_TMPL_OVERRIDE) {
			test->tmpl->pos = test->pos;
			test->tmpl->pos &= ~(POS_NEGATE|POS_NOT|POS_MARK_JUMP);
			test->tmpl->offset = test->offset;
			if (test->offset != 0 && !(test->pos & (POS_SCANFIRST|POS_SCANALL|POS_ABSOLUTE))) {
				test->tmpl->pos |= POS_SCANALL;
			}
			if (test->cbarrier) {
				test->tmpl->cbarrier = test->cbarrier;
			}
			if (test->barrier) {
				test->tmpl->barrier = test->barrier;
			}
		}
		Cohort *cdeep = 0;
		cohort = runContextualTest(sWindow, position, test->tmpl, &cdeep, origin);
		if (test->pos & POS_TMPL_OVERRIDE) {
			test->tmpl->pos = orgpos;
			test->tmpl->offset = orgoffset;
			test->tmpl->cbarrier = orgcbar;
			test->tmpl->barrier = orgbar;
			// ToDo: Being in a different window is not strictly a problem, so fix this assumption...
			if (cdeep && test->offset != 0) {
				int32_t reloff = int32_t(cdeep->local_number) - int32_t(position);
				if (!(test->pos & (POS_SCANFIRST|POS_SCANALL|POS_ABSOLUTE))) {
					if (cdeep->parent != sWindow || reloff != test->offset) {
						cohort = 0;
					}
				}
				if (!(test->pos & POS_PASS_ORIGIN)) {
					if (test->offset < 0 && reloff >= 0) {
						cohort = 0;
					}
					else if (test->offset > 0 && reloff <= 0) {
						cohort = 0;
					}
				}
			}
		}
		if (cohort && cdeep && test->linked) {
			cohort = runContextualTest(cdeep->parent, cdeep->local_number, test->linked, &cdeep, origin);
		}
		if (deep) {
			*deep = cdeep;
		}
	}
	else if (!test->ors.empty()) {
		Cohort *cdeep = 0;
		std::list<ContextualTest*>::const_iterator iter;
		for (iter = test->ors.begin() ; iter != test->ors.end() ; iter++) {
			uint32_t orgpos = (*iter)->pos;
			int32_t orgoffset = (*iter)->offset;
			uint32_t orgcbar = (*iter)->cbarrier;
			uint32_t orgbar = (*iter)->barrier;
			if (test->pos & POS_TMPL_OVERRIDE) {
				(*iter)->pos = test->pos;
				(*iter)->pos &= ~(POS_TMPL_OVERRIDE|POS_NEGATE|POS_NOT|POS_MARK_JUMP);
				(*iter)->offset = test->offset;
				if (test->offset != 0 && !(test->pos & (POS_SCANFIRST|POS_SCANALL|POS_ABSOLUTE))) {
					(*iter)->pos |= POS_SCANALL;
				}
				if (test->cbarrier) {
					(*iter)->cbarrier = test->cbarrier;
				}
				if (test->barrier) {
					(*iter)->barrier = test->barrier;
				}
			}
			cohort = runContextualTest(sWindow, position, *iter, &cdeep, origin);
			if (test->pos & POS_TMPL_OVERRIDE) {
				(*iter)->pos = orgpos;
				(*iter)->offset = orgoffset;
				(*iter)->cbarrier = orgcbar;
				(*iter)->barrier = orgbar;
				if (cdeep && test->offset != 0) {
					int32_t reloff = int32_t(cdeep->local_number) - int32_t(position);
					if (!(test->pos & (POS_SCANFIRST|POS_SCANALL|POS_ABSOLUTE))) {
						if (cdeep->parent != sWindow || reloff != test->offset) {
							cohort = 0;
						}
					}
					if (!(test->pos & POS_PASS_ORIGIN)) {
						if (test->offset < 0 && reloff >= 0) {
							cohort = 0;
						}
						else if (test->offset > 0 && reloff <= 0) {
							cohort = 0;
						}
					}
				}
			}
			if (cohort) {
				break;
			}
		}
		if (cohort && cdeep && test->linked) {
			cohort = runContextualTest(cdeep->parent, cdeep->local_number, test->linked, &cdeep, origin);
		}
		if (deep) {
			*deep = cdeep;
		}
	}
	else {
		cohort = getCohortInWindow(sWindow, position, test, pos);
	}

	CohortIterator *it = 0;
	if (!cohort) {
		retval = false;
	}
	else if (test->tmpl || !test->ors.empty()) {
		// nothing...
	}
	else {
		if (test->pos & POS_PASS_ORIGIN) {
			origin = sWindow->cohorts[0];
		}
		if (deep) {
			*deep = cohort;
		}
		if (test->pos & POS_DEP_PARENT) {
			it = &depParentIters[ci_depths[3]++];
		}
		else if (test->pos & (POS_DEP_CHILD|POS_DEP_SIBLING)) {
			Cohort *nc = runDependencyTest(sWindow, cohort, test, deep, origin);
			if (nc) {
				cohort = nc;
				retval = true;
				sWindow = cohort->parent;
				pos = (int32_t)cohort->local_number;
			}
			else {
				retval = false;
			}
			if (test->pos & POS_NONE) {
				retval = !retval;
			}
		}
		else if (test->pos & (POS_LEFT_PAR|POS_RIGHT_PAR)) {
			Cohort *nc = runParenthesisTest(sWindow, cohort, test, deep, origin);
			if (nc) {
				cohort = nc;
				retval = true;
				pos = (int32_t)cohort->local_number;
			}
			else {
				retval = false;
			}
		}
		else if (test->pos & POS_RELATION) {
			Cohort *nc = runRelationTest(sWindow, cohort, test, deep, origin);
			if (nc) {
				cohort = nc;
				retval = true;
				pos = (int32_t)cohort->local_number;
			}
			else {
				retval = false;
			}
			if (test->pos & POS_NONE) {
				retval = !retval;
			}
		}
		else if (test->offset == 0 && (test->pos & (POS_SCANFIRST|POS_SCANALL))) {
			SingleWindow *right, *left;
			int32_t rpos, lpos;

			right = left = sWindow;
			rpos = lpos = pos;

			bool brk = false;
			if (test->pos & POS_SELF) {
				cohort = runSingleTest(cohort, test, &brk, &retval, deep, origin);
			}
			if (brk && retval) {
				goto label_gotACohort;
			}

			for (uint32_t i=1 ; left || right ; i++) {
				if (left) {
					brk = false;
					cohort = runSingleTest(left, lpos-i, test, &brk, &retval, deep, origin);
					if (brk && retval) {
						goto label_gotACohort;
					}
					else if (brk) {
						left = 0;
						if (test->pos & POS_NOT) {
							right = 0;
						}
					}
					else if (lpos-i == 0) {
						if ((test->pos & (POS_SPAN_BOTH|POS_SPAN_LEFT) || always_span)) {
							left = left->previous;
							if (left) {
								lpos = i+left->cohorts.size();
							}
						}
						else {
							left = 0;
						}
					}
				}
				if (right) {
					brk = false;
					cohort = runSingleTest(right, rpos+i, test, &brk, &retval, deep, origin);
					if (brk && retval) {
						goto label_gotACohort;
					}
					else if (brk) {
						right = 0;
						if (test->pos & POS_NOT) {
							left = 0;
						}
					}
					else if (rpos+i == right->cohorts.size()-1) {
						if ((test->pos & (POS_SPAN_BOTH|POS_SPAN_RIGHT) || always_span)) {
							right = right->next;
							rpos = (0-i)-1;
						}
						else {
							right = 0;
						}
					}
				}
			}
		}
		else if (test->offset < 0) {
			it = &topologyLeftIters[ci_depths[1]++];
		}
		else if (test->offset > 0) {
			it = &topologyRightIters[ci_depths[2]++];
		}
		else {
			it = &cohortIterators[ci_depths[0]++];
		}

		if (it) {
			it->reset(cohort, test, always_span);
			Cohort *nc = 0;
			bool brk = false;
			size_t seen = 0;
			if (test->pos & POS_SELF) {
				++seen;
				nc = runSingleTest(cohort, test, &brk, &retval, deep, origin);
			}
			if (!brk) {
				for (; *it != CohortIterator(0) ; ++(*it)) {
					++seen;
					nc = runSingleTest(**it, test, &brk, &retval, deep, origin);
					if (test->pos & POS_ALL && !retval) {
						nc = 0;
						break;
					}
					if (test->pos & POS_NONE && retval) {
						nc = 0;
						break;
					}
					if (brk) {
						break;
					}
				}
			}
			if (seen == 0 && test->pos & POS_NONE) {
				retval = true;
				nc = cohort;
			}
			cohort = nc;
		}
	}

label_gotACohort:
	if (!cohort) {
		retval = false;
	}
	if (!cohort && (test->pos & POS_NOT) && !test->linked) {
		retval = !retval;
	}
	if (test->pos & POS_NEGATE) {
		retval = !retval;
	}
	if (retval) {
		test->num_match++;
	}
	else {
		test->num_fail++;
	}

	if (statistics) {
		ticks tmp = getticks();
		test->total_time += elapsed(tmp, tstamp);
	}

	if (!retval) {
		cohort = 0;
	}
	else if (!cohort) {
		cohort = sWindow->cohorts[0];
	}

	return cohort;
}

Cohort *GrammarApplicator::runDependencyTest(SingleWindow *sWindow, Cohort *current, const ContextualTest *test, Cohort **deep, Cohort *origin, const Cohort *self) {
	Cohort *rv = 0;
	Cohort *tmc = 0;

	if (self) {
		if (self == current) {
			return 0;
		}
	}
	else {
		self = current;
	}

	// ToDo: Make the dep_deep_seen key a composite of cohort number and test hash so we don't have to clear as often
	if (test->pos & POS_DEP_DEEP) {
		if (index_matches(dep_deep_seen, current->global_number)) {
			return 0;
		}
		dep_deep_seen.insert(current->global_number);
	}

	bool retval = false;
	bool brk = false;
	if (test->pos & POS_SELF) {
		tmc = runSingleTest(current, test, &brk, &retval, deep, origin);
		if (retval) {
			return tmc;
		}
	}

	if (test->pos & POS_DEP_PARENT) {
		if (current->dep_parent == std::numeric_limits<uint32_t>::max()) {
			return 0;
		}
		if (sWindow->parent->cohort_map.find(current->dep_parent) == sWindow->parent->cohort_map.end()) {
			if (verbosity_level > 0) {
				u_fprintf(ux_stderr, "Warning: Parent dependency %u -> %u does not exist - ignoring.\n", current->dep_self, current->dep_parent);
				u_fflush(ux_stderr);
			}
			return 0;
		}

		Cohort *cohort = sWindow->parent->cohort_map.find(current->dep_parent)->second;
		if (current->dep_parent == 0) {
			cohort = current->parent->cohorts[0];
		}
		bool good = true;
		if (current->parent != cohort->parent) {
			if ((!(test->pos & (POS_SPAN_BOTH|POS_SPAN_LEFT))) && cohort->parent->number < current->parent->number) {
				good = false;
			}
			else if ((!(test->pos & (POS_SPAN_BOTH|POS_SPAN_RIGHT))) && cohort->parent->number > current->parent->number) {
				good = false;
			}

		}
		if (good) {
			tmc = runSingleTest(cohort, test, &brk, &retval, deep, origin);
		}
		if (retval) {
			rv = cohort;
		}
		else if (test->pos & POS_DEP_DEEP) {
			tmc = runDependencyTest(cohort->parent, cohort, test, deep, origin, self);
			if (tmc) {
				rv = tmc;
			}
		}
	}
	else {
		const uint32HashSet *deps = 0;
		if (test->pos & POS_DEP_CHILD) {
			deps = &current->dep_children;
		}
		else {
			if (current->dep_parent == 0) {
				deps = &(current->parent->cohorts[0]->dep_children);
			}
			else {
				std::map<uint32_t,Cohort*>::iterator it = current->parent->parent->cohort_map.find(current->dep_parent);
				if (current && current->parent && current->parent->parent
					&& it != current->parent->parent->cohort_map.end()
					&& it->second
					&& !it->second->dep_children.empty()
					) {
					deps = &(it->second->dep_children);
				}
				else {
					if (verbosity_level > 0) {
						u_fprintf(ux_stderr, "Warning: Cohort %u (parent %u) did not have any siblings.\n", current->dep_self, current->dep_parent);
						u_fflush(ux_stderr);
					}
					return 0;
				}
			}
		}

		const_foreach (uint32HashSet, *deps, dter, dter_end) {
			if (*dter == current->global_number) {
				continue;
			}
			if (sWindow->parent->cohort_map.find(*dter) == sWindow->parent->cohort_map.end()) {
				if (verbosity_level > 0) {
					if (test->pos & POS_DEP_CHILD) {
						u_fprintf(ux_stderr, "Warning: Child dependency %u -> %u does not exist - ignoring.\n", current->dep_self, *dter);
					}
					else {
						u_fprintf(ux_stderr, "Warning: Sibling dependency %u -> %u does not exist - ignoring.\n", current->dep_self, *dter);
					}
					u_fflush(ux_stderr);
				}
				continue;
			}
			Cohort *cohort = sWindow->parent->cohort_map.find(*dter)->second;
			bool good = true;
			if (current->parent != cohort->parent) {
				if ((!(test->pos & (POS_SPAN_BOTH|POS_SPAN_LEFT))) && cohort->parent->number < current->parent->number) {
					good = false;
				}
				else if ((!(test->pos & (POS_SPAN_BOTH|POS_SPAN_RIGHT))) && cohort->parent->number > current->parent->number) {
					good = false;
				}

			}
			if (good) {
				tmc = runSingleTest(cohort, test, &brk, &retval, deep, origin);
			}
			if (test->pos & POS_ALL) {
				if (!retval) {
					rv = 0;
					break;
				}
				else {
					rv = cohort;
				}
			}
			else if (retval) {
				rv = cohort;
				break;
			}
			else if (test->pos & POS_DEP_DEEP) {
				tmc = runDependencyTest(cohort->parent, cohort, test, deep, origin, self);
				if (tmc) {
					rv = tmc;
					break;
				}
			}
		}
	}

	return rv;
}

Cohort *GrammarApplicator::runParenthesisTest(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test, Cohort **deep, Cohort *origin) {
	if (current->local_number < par_left_pos || current->local_number > par_right_pos) {
		return 0;
	}
	Cohort *rv = 0;

	bool retval = false;
	bool brk = false;
	Cohort *cohort = 0;
	if (test->pos & POS_LEFT_PAR) {
		cohort = sWindow->cohorts[par_left_pos];
	}
	else if (test->pos & POS_RIGHT_PAR) {
		cohort = sWindow->cohorts[par_right_pos];
	}
	runSingleTest(cohort, test, &brk, &retval, deep, origin);
	if (retval) {
		rv = cohort;
	}

	return rv;
}

Cohort *GrammarApplicator::runRelationTest(SingleWindow *sWindow, Cohort *current, const ContextualTest *test, Cohort **deep, Cohort *origin) {
	if (!(current->type & CT_RELATED) || current->relations.empty()) {
		return 0;
	}
	Cohort *rv = 0;

	bool retval = false;
	bool brk = false;
	Cohort *cohort = 0;

	if (test->relation == grammar->tag_any) {
		const_foreach (RelationCtn, current->relations, riter, riter_end) {
			const_foreach (uint32Set, riter->second, citer, citer_end) {
				std::map<uint32_t,Cohort*>::iterator it = sWindow->parent->cohort_map.find(*citer);
				if (it != sWindow->parent->cohort_map.end()) {
					cohort = it->second;
					runSingleTest(cohort, test, &brk, &retval, deep, origin);
					if (test->pos & POS_ALL) {
						if (!retval) {
							rv = 0;
							break;
						}
						else {
							rv = cohort;
						}
					}
					else if (retval) {
						rv = cohort;
						break;
					}
				}
			}
		}
	}
	else {
		RelationCtn::const_iterator riter = current->relations.find(test->relation);
		if (riter != current->relations.end()) {
			const_foreach (uint32Set, riter->second, citer, citer_end) {
				std::map<uint32_t,Cohort*>::iterator it = sWindow->parent->cohort_map.find(*citer);
				if (it != sWindow->parent->cohort_map.end()) {
					cohort = it->second;
					runSingleTest(cohort, test, &brk, &retval, deep, origin);
					if (test->pos & POS_ALL) {
						if (!retval) {
							rv = 0;
							break;
						}
						else {
							rv = cohort;
						}
					}
					else if (retval) {
						rv = cohort;
						break;
					}
				}
			}
		}
	}

	return rv;
}

}
