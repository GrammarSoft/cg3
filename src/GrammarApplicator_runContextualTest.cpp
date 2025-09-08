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

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"
#include "ContextualTest.hpp"

namespace CG3 {

Cohort* GrammarApplicator::runSingleTest(Cohort* cohort, const ContextualTest* test, uint8_t& rvs, bool* retval, Cohort** deep, Cohort* origin) {
	uint8_t regexgrpz = context_stack.back().regexgrp_ct;
	if (test->pos & POS_MARK_SET) {
		set_mark(cohort);
	}
	if (test->pos & POS_ATTACH_TO) {
		if (get_attach_to().cohort != cohort) {
			// Clear readings for rules that care about readings
			ReadingList* lists[4] = { &cohort->readings };
			if (test->pos & POS_LOOK_DELETED) {
				lists[1] = &cohort->deleted;
			}
			if (test->pos & POS_LOOK_DELAYED) {
				lists[2] = &cohort->delayed;
			}
			if (test->pos & POS_LOOK_IGNORED) {
				lists[3] = &cohort->ignored;
			}

			for (auto list : lists) {
				if (list == nullptr) {
					continue;
				}
				for (auto reading : *list) {
					reading->matched_target = false;
					reading->matched_tests = false;
				}
			}
		}
	}
	if (test->pos & POS_WITH) {
		merge_with = cohort;
	}
	if (deep) {
		*deep = cohort;
	}

	dSMC_Context context = { test, deep, origin, test->pos };

	if (test->pos & POS_CAREFUL) {
		*retval = doesSetMatchCohortCareful(*cohort, test->target, &context);
		if (!context.matched_target && (test->pos & POS_SCANFIRST)) {
			context.did_test = true;
			// Intentionally ignoring return value to set up context.matched_target
			doesSetMatchCohortNormal(*cohort, test->target, &context);
		}
	}
	else {
		*retval = doesSetMatchCohortNormal(*cohort, test->target, &context);
	}

	if (origin && (test->offset != 0 || (test->pos & (POS_SCANALL | POS_SCANFIRST))) && origin == cohort && origin->local_number != 0) {
		cohort = nullptr;
		rvs |= TRV_BREAK;
	}
	if (context.matched_target && (test->pos & POS_SCANFIRST)) {
		rvs |= TRV_BREAK;
	}
	else if (!(test->pos & (POS_SCANALL | POS_SCANFIRST | POS_DEP_DEEP | POS_DEP_GLOB))) {
		rvs |= TRV_BREAK | TRV_BREAK_DEFAULT;
	}

	bool broken = (rvs & TRV_BREAK) != 0;

	context.test = nullptr;
	context.deep = nullptr;
	context.origin = nullptr;
	context.did_test = true;
	if (test->barrier && cohort) {
		dSMC_Context context = { nullptr, nullptr, nullptr, test->pos & ~POS_CAREFUL, false, false, false, true };
		bool barrier = doesSetMatchCohortNormal(*cohort, test->barrier, &context);
		if (barrier) {
			seen_barrier = true;
			rvs |= TRV_BREAK | TRV_BARRIER;
			rvs &= ~TRV_BREAK_DEFAULT;
		}
	}
	if (test->cbarrier && cohort) {
		dSMC_Context context = { nullptr, nullptr, nullptr, test->pos | POS_CAREFUL, false, false, false, true };
		bool cbarrier = doesSetMatchCohortCareful(*cohort, test->cbarrier, &context);
		if (cbarrier) {
			seen_barrier = true;
			rvs |= TRV_BREAK | TRV_BARRIER;
			rvs &= ~TRV_BREAK_DEFAULT;
		}
	}
	if (context.matched_target && *retval) {
		rvs |= TRV_BREAK;
	}
	if (!broken && (rvs & TRV_BARRIER) && (test->pos & MASK_SELF_NB) == MASK_SELF_NB) {
		rvs &= ~(TRV_BREAK | TRV_BARRIER);
	}
	if (!*retval) {
		context_stack.back().regexgrp_ct = regexgrpz;
	}
	return cohort;
}

Cohort* GrammarApplicator::runSingleTest(SingleWindow* sWindow, size_t i, const ContextualTest* test, uint8_t& rvs, bool* retval, Cohort** deep, Cohort* origin) {
	if (i >= sWindow->cohorts.size()) {
		rvs |= TRV_BREAK;
		*retval = false;
		return 0;
	}
	Cohort* cohort = sWindow->cohorts[i];
	return runSingleTest(cohort, test, rvs, retval, deep, origin);
}

Cohort* getCohortInWindow(SingleWindow*& sWindow, size_t position, const ContextualTest* test, int32_t& pos) {
	Cohort* cohort = nullptr;
	pos = SI32(position) + test->offset;
	// ToDo: (NOT*) and (*C) tests can be cached
	if ((test->pos & POS_ABSOLUTE) && (test->pos & (POS_SPAN_LEFT|POS_SPAN_RIGHT))) {
		if (sWindow->previous && (test->pos & POS_SPAN_LEFT)) {
			sWindow = sWindow->previous;
		}
		else if (sWindow->next && (test->pos & POS_SPAN_RIGHT)) {
			sWindow = sWindow->next;
		}
		else {
			return cohort;
		}
	}
	if (test->pos & POS_ABSOLUTE) {
		if (test->offset < 0) {
			pos = SI32(sWindow->cohorts.size()) + test->offset;
		}
		else {
			pos = test->offset;
		}
	}
	if (pos >= 0) {
		if (pos >= SI32(sWindow->cohorts.size()) && (test->pos & (POS_SPAN_RIGHT | POS_SPAN_BOTH)) && sWindow->next) {
			sWindow = sWindow->next;
			pos = 0;
		}
	}
	else {
		if ((test->pos & (POS_SPAN_LEFT | POS_SPAN_BOTH)) && sWindow->previous) {
			sWindow = sWindow->previous;
			pos = SI32(sWindow->cohorts.size()) - 1;
		}
	}
	if (pos >= 0 && pos < SI32(sWindow->cohorts.size())) {
		cohort = sWindow->cohorts[pos];
	}
	return cohort;
}

bool GrammarApplicator::posOutputHelper(const SingleWindow* sWindow, size_t position, const ContextualTest* test, const Cohort* cohort, const Cohort* cdeep) {
	bool good = false;

	const Cohort* cs[4] = {
		cohort,
		cdeep,
		cohort,
		cdeep,
	};
	if (tmpl_cntx.min) {
		cs[2] = tmpl_cntx.min;
	}
	if (tmpl_cntx.max) {
		cs[3] = tmpl_cntx.max;
	}

	std::sort(cs, cs + 4, compare_Cohort());

	// If the override included * or @, don't care about offsets
	if (test->pos & (POS_SCANFIRST | POS_SCANALL | POS_ABSOLUTE)) {
		good = true;
	}
	else {
		// ...otherwise, positive offsets need to match the leftmost of entry/exit
		if (test->offset > 0 && SI32(cs[0]->local_number) - SI32(position) == test->offset) {
			good = true;
		}
		// ...and, negative offsets need to match the rightmost of entry/exit
		else if (test->offset < 0 && SI32(cs[3]->local_number) - SI32(position) == test->offset) {
			good = true;
		}
	}
	if (!(test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT | POS_SPAN_RIGHT)) && cdeep->parent != sWindow) {
		good = false;
	}
	if (!(test->pos & POS_PASS_ORIGIN)) {
		if (test->offset < 0 && cs[3]->local_number > position) {
			good = false;
		}
		else if (test->offset > 0 && cs[0]->local_number < position) {
			good = false;
		}
	}
	return good;
}

Cohort* GrammarApplicator::runContextualTest_tmpl(SingleWindow* sWindow, size_t position, const ContextualTest* test, ContextualTest* tmpl, Cohort*& cdeep, Cohort* origin) {
	Cohort* min = tmpl_cntx.min;
	Cohort* max = tmpl_cntx.max;
	bool in_template = tmpl_cntx.in_template;
	tmpl_cntx.in_template = true;
	if (test->linked) {
		tmpl_cntx.linked.push_back(test->linked);
	}

	uint64_t orgpos = tmpl->pos;
	int32_t orgoffset = tmpl->offset;
	uint32_t orgcbar = tmpl->cbarrier;
	uint32_t orgbar = tmpl->barrier;
	if (test->pos & POS_TMPL_OVERRIDE) {
		tmpl->pos = test->pos;
		tmpl->pos &= ~(POS_NEGATE | POS_NOT | POS_JUMP);
		tmpl->offset = test->offset;
		if (test->offset != 0 && !(test->pos & (POS_SCANFIRST | POS_SCANALL | POS_ABSOLUTE))) {
			tmpl->pos |= POS_SCANALL;
		}
		if (test->cbarrier) {
			tmpl->cbarrier = test->cbarrier;
		}
		if (test->barrier) {
			tmpl->barrier = test->barrier;
		}
	}
	Cohort* cohort = runContextualTest(sWindow, position, tmpl, &cdeep, origin);
	if (test->pos & POS_TMPL_OVERRIDE) {
		tmpl->pos = orgpos;
		tmpl->offset = orgoffset;
		tmpl->cbarrier = orgcbar;
		tmpl->barrier = orgbar;
		if (cohort && cdeep && test->offset != 0 && !posOutputHelper(sWindow, position, test, cohort, cdeep)) {
			cohort = nullptr;
		}
	}

	if (test->linked) {
		tmpl_cntx.linked.pop_back();
	}
	if (!cohort) {
		tmpl_cntx.min = min;
		tmpl_cntx.max = max;
		tmpl_cntx.in_template = in_template;
	}

	return cohort;
}

Cohort* GrammarApplicator::runContextualTest(SingleWindow* sWindow, size_t position, const ContextualTest* test, Cohort** deep, Cohort* origin) {
	if (test->pos & POS_UNKNOWN) {
		u_fprintf(ux_stderr, "Error: Contextual tests with position '?' cannot be used directly. Provide an override position.\n");
		CG3Quit(1);
	}

	Cohort* cohort = nullptr;
	bool retval = true;
	auto orgSWin = sWindow;

	if (test->pos & POS_JUMP) {
		Cohort* j = nullptr;
		if (test->jump_pos == JUMP_MARK) {
			j = get_mark();
		}
		else if (test->jump_pos == JUMP_ATTACH) {
			j = get_attach_to().cohort;
		}
		else if (test->jump_pos == JUMP_TARGET) {
			for (auto& it : reversed(context_stack)) {
				if (it.is_with) {
					j = it.target.cohort;
				}
			}
		}
		else {
			if (context_stack.size() > 1) {
				auto& ctx = context_stack[context_stack.size() - 2];
				if (ctx.context.size() >= UIZ(test->jump_pos)) {
					j = ctx.context[test->jump_pos - 1];
				}
			}
		}
		if (j) {
			sWindow = j->parent;
			position = j->local_number;
		}
		else {
			retval = false;
		}
	}
	int32_t pos = SI32(position) + test->offset;
	if (!retval) {
		// jump failed because position does not exist
		goto label_gotACohort;
	}

	if (test->tmpl) {
		Cohort* cdeep = nullptr;
		cohort = runContextualTest_tmpl(sWindow, position, test, test->tmpl, cdeep, origin);
		if (deep) {
			*deep = cdeep;
		}
	}
	else if (!test->ors.empty()) {
		Cohort* cdeep = nullptr;
		for (auto iter : test->ors) {
			dep_deep_seen.clear();
			cohort = runContextualTest_tmpl(sWindow, position, test, iter, cdeep, origin);
			if (cohort) {
				break;
			}
		}
		if (deep) {
			*deep = cdeep;
		}
	}
	else {
		cohort = getCohortInWindow(sWindow, position, test, pos);
	}

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
		if (tmpl_cntx.in_template) {
			auto gpos = make_64(cohort->parent->number, cohort->local_number);
			if (tmpl_cntx.min == 0 || gpos < make_64(tmpl_cntx.min->parent->number, tmpl_cntx.min->local_number)) {
				tmpl_cntx.min = cohort;
			}
			if (tmpl_cntx.max == 0 || gpos > make_64(tmpl_cntx.max->parent->number, tmpl_cntx.max->local_number)) {
				tmpl_cntx.max = cohort;
			}
			if (deep) {
				auto gpos = make_64((*deep)->parent->number, (*deep)->local_number);
				if (tmpl_cntx.min == 0 || gpos < make_64(tmpl_cntx.min->parent->number, tmpl_cntx.min->local_number)) {
					tmpl_cntx.min = *deep;
				}
				if (tmpl_cntx.max == 0 || gpos > make_64(tmpl_cntx.max->parent->number, tmpl_cntx.max->local_number)) {
					tmpl_cntx.max = *deep;
				}
			}
		}

		CohortIterator* it = nullptr;
		if ((test->pos & POS_DEP_PARENT) && (test->pos & POS_DEP_GLOB)) {
			it = &depAncestorIters[ci_depths[5]++];
		}
		else if (test->pos & POS_DEP_PARENT) {
			it = &depParentIters[ci_depths[3]++];
		}
		else if (test->pos & POS_DEP_GLOB) {
			it = &depDescendentIters[ci_depths[4]++];
		}
		else if (test->pos & (POS_DEP_CHILD | POS_DEP_SIBLING)) {
			Cohort* nc = runDependencyTest(sWindow, cohort, test, deep, origin, 0);
			if (nc) {
				cohort = nc;
				retval = true;
				sWindow = cohort->parent;
			}
			else {
				retval = false;
			}
			if (test->pos & POS_NONE) {
				retval = !retval;
			}
		}
		else if (test->pos & (POS_LEFT_PAR | POS_RIGHT_PAR)) {
			Cohort* nc = runParenthesisTest(sWindow, cohort, test, deep, origin);
			if (nc) {
				cohort = nc;
				retval = true;
			}
			else {
				retval = false;
			}
		}
		else if (test->pos & POS_RELATION) {
			Cohort* nc = runRelationTest(sWindow, cohort, test, deep, origin);
			if (nc) {
				cohort = nc;
				retval = true;
			}
			else {
				retval = false;
			}
			if (test->pos & POS_NONE) {
				retval = !retval;
			}
		}
		else if (test->pos & POS_BAG_OF_TAGS) {
			bool match = doesSetMatchReading(sWindow->bag_of_tags, test->target, true);
			if (!match && (test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT | POS_SPAN_RIGHT))) {
				SingleWindow *left = sWindow->previous, *right = sWindow->next;
				while (left || right) {
					if (left && (test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT))) {
						match = doesSetMatchReading(left->bag_of_tags, test->target, true);
						left = left->previous;
					}
					else {
						left = nullptr;
					}
					if (right && (test->pos & (POS_SPAN_BOTH | POS_SPAN_RIGHT))) {
						match = doesSetMatchReading(right->bag_of_tags, test->target, true);
						right = right->next;
					}
					else {
						right = nullptr;
					}
					if (match) {
						break;
					}
				}
			}
			if (test->pos & POS_NOT) {
				match = !match;
			}
			if (match) {
				if (test->linked) {
					cohort = runContextualTest(sWindow, position, test->linked, deep, origin);
				}
			}
			else {
				retval = false;
			}
		}
		else if (test->offset == 0 && (test->pos & (POS_SCANFIRST | POS_SCANALL))) {
			SingleWindow *right, *left;
			int32_t rpos, lpos;

			right = left = sWindow;
			rpos = lpos = pos;

			uint8_t rvs = 0;
			if (test->pos & POS_SELF) {
				cohort = runSingleTest(cohort, test, rvs, &retval, deep, origin);
				if (!retval && (rvs & TRV_BREAK_DEFAULT)) {
					rvs &= ~(TRV_BREAK | TRV_BREAK_DEFAULT);
				}
			}
			if ((rvs & TRV_BREAK) && retval) {
				goto label_gotACohort;
			}

			for (uint32_t i = 1; left || right; ++i) {
				if (left) {
					rvs = 0;
					cohort = runSingleTest(left, lpos - i, test, rvs, &retval, deep, origin);
					if ((rvs & TRV_BREAK) && retval) {
						goto label_gotACohort;
					}
					else if (rvs & TRV_BREAK) {
						left = 0;
						if (test->pos & POS_NOT) {
							right = nullptr;
						}
					}
					else if (lpos - i == 0) {
						if ((test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT) || always_span)) {
							left = left->previous;
							if (left) {
								lpos = SI32(i + left->cohorts.size());
							}
						}
						else {
							left = nullptr;
						}
					}
				}
				if (right) {
					rvs = 0;
					cohort = runSingleTest(right, rpos + i, test, rvs, &retval, deep, origin);
					if ((rvs & TRV_BREAK) && retval) {
						goto label_gotACohort;
					}
					else if (rvs & TRV_BREAK) {
						right = 0;
						if (test->pos & POS_NOT) {
							left = nullptr;
						}
					}
					else if (rpos + i == right->cohorts.size() - 1) {
						if ((test->pos & (POS_SPAN_BOTH | POS_SPAN_RIGHT) || always_span)) {
							right = right->next;
							rpos = (0 - i) - 1;
						}
						else {
							right = nullptr;
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
			Cohort* nc = nullptr;
			uint8_t rvs = 0;
			size_t seen = 0;
			if ((test->pos & POS_SELF) && (!(test->pos & MASK_POS_LORR) || ((test->pos & POS_DEP_PARENT) && !(test->pos & POS_DEP_GLOB)))) {
				++seen;
				assert(position < orgSWin->cohorts.size() && "Somehow, the input position wasn't inside the current window.");
				Cohort* self = orgSWin->cohorts[position];
				nc = runSingleTest(self, test, rvs, &retval, deep, origin);
				if (!retval && (rvs & TRV_BREAK_DEFAULT)) {
					rvs &= ~(TRV_BREAK | TRV_BREAK_DEFAULT);
				}
			}
			if (!(rvs & TRV_BREAK)) {
				Cohort* current = cohort;
				for (; *it != CohortIterator(0); ++(*it)) {
					++seen;
					if ((test->pos & POS_LEFT) && less_Cohort(current, **it)) {
						nc = nullptr;
						retval = false;
						break;
					}
					if ((test->pos & POS_RIGHT) && !less_Cohort(current, **it)) {
						nc = nullptr;
						retval = false;
						break;
					}
					nc = runSingleTest(**it, test, rvs, &retval, deep, origin);
					if (test->pos & POS_ALL && !retval) {
						nc = nullptr;
						break;
					}
					if (test->pos & POS_NONE && retval) {
						nc = nullptr;
						break;
					}
					if (rvs & TRV_BREAK) {
						break;
					}
					current = **it;
				}
			}
			if (seen == 0) {
				retval = false;
			}
			if (!retval && test->pos & POS_NONE) {
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

	/*
	if (profiler) {
		auto it = profiler->contexts.find(test->hash);
		if (it != profiler->contexts.end()) {
			if (retval) {
				++it->second.num_match;
				auto rc = std::make_pair(current_rule->number + 1, test->hash);
				++profiler->rule_contexts[rc];
			}
			else {
				++it->second.num_fail;
			}
		}
	}
	//*/

	if (!retval) {
		cohort = nullptr;
	}
	else if (!cohort) {
		cohort = sWindow->cohorts[0];
	}

	return cohort;
}

Cohort* GrammarApplicator::runDependencyTest(SingleWindow* sWindow, Cohort* current, const ContextualTest* test, Cohort** deep, Cohort* origin, const Cohort* self) {
	Cohort* rv = nullptr;

	if (self) {
		if (self == current) {
			return 0;
		}
	}
	else {
		self = current;
	}

	// ToDo: Now that dep_deep_seen is a composite, investigate all .clear() to see if they're needed
	if (test->pos & POS_DEP_DEEP) {
		if (dep_deep_seen.contains(std::make_pair(test->hash, current->global_number))) {
			return 0;
		}
		dep_deep_seen.insert(std::make_pair(test->hash, current->global_number));
	}

	if ((test->pos & POS_SELF) && !(test->pos & MASK_POS_LORR)) {
		bool retval = false;
		uint8_t rvs = 0;
		Cohort* tmc = runSingleTest(current, test, rvs, &retval, deep, origin);
		if (retval) {
			return tmc;
		}
		if (rvs & TRV_BARRIER) {
			return 0;
		}
	}

	// Recursion may happen, so can't be static
	uint32SortedVector tmp_deps;
	uint32SortedVector* deps = nullptr;
	if (test->pos & POS_DEP_CHILD) {
		deps = &current->dep_children;
	}
	else {
		if (current->dep_parent == 0) {
			deps = &(current->parent->cohorts[0]->dep_children);
		}
		else {
			auto it = current->parent->parent->cohort_map.find(current->dep_parent);
			if (it != current->parent->parent->cohort_map.end() && it->second && !it->second->dep_children.empty()) {
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

	// ToDo: This whole function could resolve cohorts earlier and skip doing it twice
	if (test->pos & MASK_POS_LORR) {
		// I think this way around makes most sense? Loop over the container that's slower to look up in. But tests will show.
		for (const auto& iter : sWindow->parent->cohort_map) {
			if (deps->count(iter.second->global_number)) {
				if (test->pos & POS_LEFT) {
					if (less_Cohort(iter.second, current)) {
						tmp_deps.insert(iter.second->global_number);
					}
				}
				else if ((test->pos & POS_RIGHT)) {
					if (less_Cohort(current, iter.second)) {
						tmp_deps.insert(iter.second->global_number);
					}
				}
				else {
					tmp_deps.insert(iter.second->global_number);
				}
			}
		}

		if (test->pos & POS_SELF) {
			tmp_deps.insert(current->global_number);
		}
		if ((test->pos & POS_RIGHTMOST) && !tmp_deps.empty()) {
			uint32SortedVector::container& cont = tmp_deps.get();
			std::reverse(cont.begin(), cont.end());
		}

		deps = &tmp_deps;
	}

	for (auto dter : *deps) {
		if (dter == current->global_number && !(test->pos & POS_SELF)) {
			continue;
		}
		if (sWindow->parent->cohort_map.find(dter) == sWindow->parent->cohort_map.end()) {
			if (verbosity_level > 0) {
				if (test->pos & POS_DEP_CHILD) {
					u_fprintf(ux_stderr, "Warning: Child dependency %u -> %u does not exist - ignoring.\n", current->dep_self, dter);
				}
				else {
					u_fprintf(ux_stderr, "Warning: Sibling dependency %u -> %u does not exist - ignoring.\n", current->dep_self, dter);
				}
				u_fflush(ux_stderr);
			}
			continue;
		}
		Cohort* cohort = sWindow->parent->cohort_map.find(dter)->second;
		if (cohort->type & CT_REMOVED) {
			continue;
		}
		bool good = true;
		if (current->parent != cohort->parent) {
			if ((!(test->pos & (POS_SPAN_BOTH | POS_SPAN_LEFT))) && cohort->parent->number < current->parent->number) {
				good = false;
			}
			else if ((!(test->pos & (POS_SPAN_BOTH | POS_SPAN_RIGHT))) && cohort->parent->number > current->parent->number) {
				good = false;
			}
		}
		bool retval = false;
		uint8_t rvs = 0;
		if (good) {
			runSingleTest(cohort, test, rvs, &retval, deep, origin);
		}
		if (test->pos & POS_ALL) {
			if (!retval) {
				rv = nullptr;
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
		else if (rvs & TRV_BARRIER) {
			continue;
		}
		else if (test->pos & POS_DEP_DEEP) {
			Cohort* tmc = runDependencyTest(cohort->parent, cohort, test, deep, origin, self);
			if (tmc) {
				rv = tmc;
				break;
			}
		}
	}

	return rv;
}

Cohort* GrammarApplicator::runParenthesisTest(SingleWindow* sWindow, const Cohort* current, const ContextualTest* test, Cohort** deep, Cohort* origin) {
	if (current->local_number < par_left_pos || current->local_number > par_right_pos) {
		return 0;
	}
	Cohort* rv = nullptr;

	bool retval = false;
	uint8_t rvs = 0;
	Cohort* cohort = nullptr;
	if (test->pos & POS_LEFT_PAR) {
		cohort = sWindow->cohorts[par_left_pos];
	}
	else {
		cohort = sWindow->cohorts[par_right_pos];
	}
	runSingleTest(cohort, test, rvs, &retval, deep, origin);
	if (retval) {
		rv = cohort;
	}

	return rv;
}

Cohort* GrammarApplicator::runRelationTest(SingleWindow* sWindow, Cohort* current, const ContextualTest* test, Cohort** deep, Cohort* origin) {
	if (!(current->type & CT_RELATED) || current->relations.empty()) {
		return 0;
	}

	// Recursion may happen, so can't be static
	CohortSet rels;
	uint8_t regexgrpz = context_stack.back().regexgrp_ct;

	auto rtag = grammar->single_tags[test->relation];
	while (rtag->type & T_VARSTRING) {
		rtag = generateVarstringTag(rtag);
	}

	if (rtag->hash == grammar->tag_any) {
		for (const auto& riter : current->relations) {
			for (auto citer : riter.second) {
				auto it = sWindow->parent->cohort_map.find(citer);
				if (it != sWindow->parent->cohort_map.end()) {
					rels.insert(it->second);
				}
			}
		}
	}
	else if (rtag->type & T_REGEXP) {
		UErrorCode status = U_ZERO_ERROR;
		auto caps = uregex_groupCount(rtag->regexp, &status);
		for (const auto& riter : current->relations) {
			for (auto citer : riter.second) {
				auto it = sWindow->parent->cohort_map.find(citer);
				if (it != sWindow->parent->cohort_map.end() && doesTagMatchRegexp(riter.first, *rtag, caps != 0)) {
					rels.insert(it->second);
					context_stack.back().regexgrp_ct = std::min(context_stack.back().regexgrp_ct, UI8(regexgrpz + caps));
				}
			}
		}
	}
	else {
		auto riter = current->relations.find(rtag->hash);
		if (riter != current->relations.end()) {
			for (auto citer : riter->second) {
				auto it = sWindow->parent->cohort_map.find(citer);
				if (it != sWindow->parent->cohort_map.end()) {
					rels.insert(it->second);
				}
			}
		}
	}

	if (test->pos & POS_LEFT) {
		static thread_local CohortSet tmp;
		tmp.assign(rels.begin(), rels.lower_bound(current));
		rels.swap(tmp);
	}
	if (test->pos & POS_RIGHT) {
		static thread_local CohortSet tmp;
		tmp.assign(rels.lower_bound(current), rels.end());
		rels.swap(tmp);
	}
	if (test->pos & POS_SELF) {
		rels.insert(current);
	}
	if ((test->pos & POS_LEFTMOST) && !rels.empty()) {
		Cohort* c = rels.front();
		rels.clear();
		rels.insert(c);
	}
	if ((test->pos & POS_RIGHTMOST) && !rels.empty()) {
		Cohort* c = rels.back();
		rels.clear();
		rels.insert(c);
	}

	Cohort* rv = nullptr;
	for (auto iter : rels) {
		uint8_t rvs = 0;
		bool retval = false;

		runSingleTest(iter, test, rvs, &retval, deep, origin);
		if (test->pos & POS_ALL) {
			if (!retval) {
				rv = nullptr;
				break;
			}
			else {
				rv = iter;
			}
		}
		else if (retval) {
			rv = iter;
			break;
		}
	}

	if (!rv) {
		context_stack.back().regexgrp_ct = regexgrpz;
	}
	return rv;
}
}
