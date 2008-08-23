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
	if (deep) {
		*deep = cohort;
	}
	*retval = doesSetMatchCohortNormal(cohort, test->target);
	bool foundfirst = *retval;
	if (test->pos & POS_CAREFUL) {
		if (*retval) {
			*retval = doesSetMatchCohortCareful(cohort, test->target);
		}
		else {
			*retval = false;
		}
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
		bool barrier = doesSetMatchCohortNormal(cohort, test->barrier);
		if (barrier) {
			*brk = true;
		}
	}
	if (test->cbarrier) {
		bool cbarrier = doesSetMatchCohortCareful(cohort, test->cbarrier);
		if (cbarrier) {
			*brk = true;
		}
	}
	if (origin && origin == cohort && origin->local_number != 0) {
		*retval = false;
		*brk = true;
	}
	if (foundfirst && *retval) {
		*brk = true;
	}
	return cohort;
}

Cohort *GrammarApplicator::runContextualTest(SingleWindow *sWindow, const size_t position, const ContextualTest *test, Cohort **deep, Cohort *origin) {
	Cohort *cohort = 0;
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
	clock_t tstamp = 0;
	int32_t pos = (int32_t)(position + test->offset);

	if (statistics) {
		tstamp = clock();
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
		if (test->offset == 0 && (test->pos & (POS_SCANFIRST|POS_SCANALL))) {
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
			if (test->pos & (POS_DEP_CHILD|POS_DEP_PARENT|POS_DEP_SIBLING)) {
				Cohort *nc = doesSetMatchDependency(sWindow, cohort, test);
				if (nc) {
					cohort = nc;
					retval = true;
					sWindow = cohort->parent;
					pos = (int32_t)cohort->local_number;
				}
				else {
					retval = false;
				}
			}
			else {
				if (test->pos & POS_CAREFUL) {
					retval = doesSetMatchCohortCareful(cohort, test->target);
				}
				else {
					retval = doesSetMatchCohortNormal(cohort, test->target);
				}
			}
			if (test->pos & POS_NEGATIVE) {
				retval = !retval;
			}
			if (retval && test->linked) {
				retval = (runContextualTest(sWindow, pos, test->linked, deep, origin) != 0);
			}
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
		test->total_time += (clock() - tstamp);
	}

	if (!retval) {
		cohort = 0;
	}
	else if (!cohort) {
		cohort = sWindow->cohorts.at(0);
	}

	return cohort;
}
