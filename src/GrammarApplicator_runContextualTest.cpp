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

#include "GrammarApplicator.h"

using namespace CG3;

Cohort *GrammarApplicator::runSingleTest(SingleWindow *sWindow, size_t i, const ContextualTest *test, bool *brk, bool *retval, Cohort **deep, Cohort *origin) {
	if (i >= sWindow->cohorts.size()) {
		*brk = true;
		*retval = false;
		return 0;
	}
	Cohort *cohort = sWindow->cohorts.at(i);
	if (test->pos & POS_MARK_SET) {
		mark = cohort;
	}
	if (deep) {
		*deep = cohort;
	}
	*retval = doesSetMatchCohortNormal(cohort, test->target, test->pos);
	bool foundfirst = *retval;
	if (test->pos & POS_CAREFUL) {
		if (*retval) {
			*retval = doesSetMatchCohortCareful(cohort, test->target, test->pos);
		}
		else {
			*retval = false;
		}
	}
	if (origin && (test->offset != 0 || (test->pos & (POS_SCANALL|POS_SCANFIRST))) && origin == cohort && origin->local_number != 0) {
		*retval = false;
		*brk = true;
	}
	if (test->pos & POS_NEGATIVE) {
		*retval = !*retval;
	}
	if (*retval && test->linked) {
		if (test->linked->pos & POS_NO_PASS_ORIGIN) {
			*retval = (runContextualTest(sWindow, cohort->local_number, test->linked, deep, cohort) != 0);
		}
		else {
			*retval = (runContextualTest(sWindow, cohort->local_number, test->linked, deep, origin) != 0);
		}
	}
	if (foundfirst && test->pos & POS_SCANFIRST) {
		*brk = true;
	}
	if (test->barrier) {
		bool barrier = doesSetMatchCohortNormal(cohort, test->barrier, test->pos);
		if (barrier) {
			*brk = true;
		}
	}
	if (test->cbarrier) {
		bool cbarrier = doesSetMatchCohortCareful(cohort, test->cbarrier, test->pos);
		if (cbarrier) {
			*brk = true;
		}
	}
	if (foundfirst && *retval) {
		*brk = true;
	}
	return cohort;
}

Cohort *GrammarApplicator::runContextualTest(SingleWindow *sWindow, size_t position, const ContextualTest *test, Cohort **deep, Cohort *origin) {
	Cohort *cohort = 0;

	if (test->pos & POS_MARK_JUMP) {
		sWindow = mark->parent;
		position = mark->local_number;
	}

	if (test->tmpl) {
		Cohort *cdeep = 0;
		cohort = runContextualTest(sWindow, position, test->tmpl, &cdeep, origin);
		if (cohort && cdeep && test->linked) {
			cohort = runContextualTest(cdeep->parent, cdeep->local_number, test->linked, &cdeep, origin);
		}
		if (deep) {
			*deep = cdeep;
		}
		return cohort;
	}
	else if (!test->ors.empty()) {
		Cohort *cdeep = 0;
		std::list<ContextualTest*>::const_iterator iter;
		for (iter = test->ors.begin() ; iter != test->ors.end() ; iter++) {
			cohort = runContextualTest(sWindow, position, *iter, &cdeep, origin);
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
		return cohort;
	}

	bool retval = true;
	ticks tstamp(gtimer);
	int32_t pos = (int32_t)(position + test->offset);

	if (statistics) {
		tstamp = getticks();
	}

	// ToDo: (NOT *) and (*C) tests can be cached
	if (test->pos & POS_ABSOLUTE) {
		if (test->offset < 0) {
			pos = ((int32_t)sWindow->cohorts.size()-1) - test->offset;
		}
		else {
			pos = test->offset;
		}
	}
	if (pos >= 0) {
		if (pos >= (int32_t)sWindow->cohorts.size() && (test->pos & (POS_SPAN_RIGHT|POS_SPAN_BOTH)) && sWindow->next) {
			sWindow = sWindow->next;
			pos = 0;
		}
	}
	else {
		if ((test->pos & (POS_SPAN_LEFT|POS_SPAN_BOTH)) && sWindow->previous) {
			sWindow = sWindow->previous;
			pos = (int32_t)sWindow->cohorts.size()-1;
		}
	}
	if (pos >= 0 && pos < (int32_t)sWindow->cohorts.size()) {
		cohort = sWindow->cohorts.at(pos);
	}

	if (!cohort) {
		retval = false;
	}
	else {
		if (test->pos & POS_PASS_ORIGIN) {
			origin = sWindow->cohorts.at(0);
		}
		if (deep) {
			*deep = cohort;
		}
		if (test->pos & (POS_DEP_CHILD|POS_DEP_PARENT|POS_DEP_SIBLING)) {
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
			if (test->pos & POS_DEP_NONE) {
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
		else if (test->offset == 0 && (test->pos & (POS_SCANFIRST|POS_SCANALL))) {
			SingleWindow *right, *left;
			int32_t rpos, lpos;

			right = left = sWindow;
			rpos = lpos = pos;

			for (uint32_t i=1 ; left || right ; i++) {
				if (left) {
					bool brk = false;
					cohort = runSingleTest(left, lpos-i, test, &brk, &retval, deep, origin);
					if (brk && retval) {
						break;
					}
					else if (brk) {
						left = 0;
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
					bool brk = false;
					cohort = runSingleTest(right, rpos+i, test, &brk, &retval, deep, origin);
					if (brk && retval) {
						break;
					}
					else if (brk) {
						right = 0;
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
		else if (test->offset < 0 && pos >= 0 && (test->pos & (POS_SCANALL|POS_SCANFIRST))) {
			for (int i=pos;i>=0;i--) {
				bool brk = false;
				cohort = runSingleTest(sWindow, i, test, &brk, &retval, deep, origin);
				if (brk) {
					break;
				}
				if (i == 0 && (test->pos & (POS_SPAN_BOTH|POS_SPAN_LEFT) || always_span) && sWindow->previous) {
					sWindow = sWindow->previous;
					i = (int32_t)sWindow->cohorts.size()-1;
				}
			}
		}
		else if (test->offset > 0 && pos <= (int32_t)sWindow->cohorts.size() && (test->pos & (POS_SCANALL|POS_SCANFIRST))) {
			for (uint32_t i=pos;i<sWindow->cohorts.size();i++) {
				bool brk = false;
				cohort = runSingleTest(sWindow, i, test, &brk, &retval, deep, origin);
				if (brk) {
					break;
				}
				if (i == sWindow->cohorts.size()-1 && (test->pos & (POS_SPAN_BOTH|POS_SPAN_RIGHT) || always_span) && sWindow->next) {
					sWindow = sWindow->next;
					i = 0;
				}
			}
		}
		else {
			bool brk = false;
			cohort = runSingleTest(sWindow, pos, test, &brk, &retval, deep, origin);
		}
	}
	if (!cohort && test->pos & POS_NEGATIVE) {
		retval = !retval;
	}
	if (test->pos & POS_NEGATED) {
		retval = !retval;
	}
	if (retval) {
		test->num_match++;
	} else {
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
		cohort = sWindow->cohorts.at(0);
	}

	return cohort;
}

Cohort *GrammarApplicator::runDependencyTest(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test, Cohort **deep, Cohort *origin) {
	Cohort *rv = 0;
	Cohort *tmc = 0;

	bool retval = false;
	bool brk = false;
	if (test->pos & POS_DEP_SELF) {
		tmc = runSingleTest(current->parent, current->local_number, test, &brk, &retval, deep, origin);
		if (retval) {
			return tmc;
		}
	}

	if (test->pos & POS_DEP_PARENT && current->dep_self != current->dep_parent) {
		if (sWindow->parent->cohort_map.find(current->dep_parent) == sWindow->parent->cohort_map.end()) {
			if (verbosity_level > 0) {
				u_fprintf(ux_stderr, "Warning: Parent dependency %u -> %u does not exist - ignoring.\n", current->dep_self, current->dep_parent);
				u_fflush(ux_stderr);
			}
			return 0;
		}

		Cohort *cohort = sWindow->parent->cohort_map.find(current->dep_parent)->second;
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
			tmc = runSingleTest(cohort->parent, cohort->local_number, test, &brk, &retval, deep, origin);
		}
		if (retval) {
			rv = cohort;
		}
		else if (test->pos & POS_DEP_DEEP) {
			tmc = runDependencyTest(cohort->parent, cohort, test, deep, origin);
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
				deps = &(current->parent->cohorts.at(0)->dep_children);
			}
			else {
				deps = &(current->parent->parent->cohort_map.find(current->dep_parent)->second->dep_children);
			}
		}

		const_foreach(uint32HashSet, *deps, dter, dter_end) {
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
				tmc = runSingleTest(cohort->parent, cohort->local_number, test, &brk, &retval, deep, origin);
			}
			if (test->pos & POS_DEP_ALL) {
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
				tmc = runDependencyTest(cohort->parent, cohort, test, deep, origin);
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
	Cohort *tmc = 0;

	bool retval = false;
	bool brk = false;
	Cohort *cohort = 0;
	if (test->pos & POS_LEFT_PAR) {
		cohort = sWindow->cohorts.at(par_left_pos);
	}
	else if (test->pos & POS_RIGHT_PAR) {
		cohort = sWindow->cohorts.at(par_right_pos);
	}
	tmc = runSingleTest(cohort->parent, cohort->local_number, test, &brk, &retval, deep, origin);
	if (retval) {
		rv = cohort;
	}

	return rv;
}
