/*
* Copyright (C) 2006, GrammarSoft Aps
* and the VISL project at the University of Southern Denmark.
* All Rights Reserved.
*
* The contents of this file are subject to the GrammarSoft Public
* License Version 1.0 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.grammarsoft.com/GSPL or
* http://visl.sdu.dk/GSPL.txt
* 
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*/

#include "GrammarApplicator.h"

using namespace CG3;

Cohort *GrammarApplicator::runSingleTest(SingleWindow *sWindow, size_t i, const ContextualTest *test, bool *brk, bool *retval) {
	Cohort *cohort = sWindow->cohorts.at(i);
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
		*retval = (runContextualTest(sWindow, cohort->local_number, test->linked) != 0);
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
	if (foundfirst && *retval) {
		*brk = true;
	}
	return cohort;
}

Cohort *GrammarApplicator::runContextualTest(SingleWindow *sWindow, const size_t position, const ContextualTest *test) {
	bool retval = true;
	clock_t tstamp = 0;
	int32_t pos = (int32_t)(position + test->offset);
	Cohort *cohort = 0;

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
		if (test->offset == 0 && (test->pos & (POS_SCANFIRST|POS_SCANALL))) {
			SingleWindow *right, *left;
			int32_t rpos, lpos;

			right = left = sWindow;
			rpos = lpos = pos;

			for (uint32_t i=1 ; left || right ; i++) {
				if (left) {
					bool brk = false;
					cohort = runSingleTest(left, lpos-i, test, &brk, &retval);
					if (brk) {
						break;
					}
					if (lpos-i == 0) {
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
					cohort = runSingleTest(right, rpos+i, test, &brk, &retval);
					if (brk) {
						break;
					}
					if (rpos+i == right->cohorts.size()-1) {
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
				cohort = runSingleTest(sWindow, i, test, &brk, &retval);
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
				cohort = runSingleTest(sWindow, i, test, &brk, &retval);
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
				retval = (runContextualTest(sWindow, pos, test->linked) != 0);
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
