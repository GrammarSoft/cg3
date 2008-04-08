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
using namespace CG3::Strings;

bool GrammarApplicator::doesTagMatchSet(const uint32_t tag, const uint32_t set) {
	bool retval = false;

	stdext::hash_map<uint32_t, Set*>::const_iterator iter = grammar->sets_by_contents.find(set);
	if (iter != grammar->sets_by_contents.end()) {
		const Set *theset = iter->second;

		if (theset->single_tags.find(tag) != theset->single_tags.end()) {
			retval = true;
		}
		else {
			CompositeTag *ctag = new CompositeTag();
			ctag->addTag(tag);
			ctag->rehash();

			if (theset->tags.find(ctag->hash) != theset->tags.end()) {
				retval = true;
			}
			delete ctag;
		}
	}
	return retval;
}

inline bool GrammarApplicator::__index_matches(const stdext::hash_map<uint32_t, uint32HashSet*> *me, const uint32_t value, const uint32_t set) {
	if (me->find(value) != me->end()) {
		const uint32HashSet *index = me->find(value)->second;
		if (index->find(set) != index->end()) {
			cache_hits++;
			return true;
		}
	}
	return false;
}

bool GrammarApplicator::doesTagMatchReading(const Reading *reading, const uint32_t ztag, bool bypass_index) {
	bool retval = false;
	bool match = true;
	bypass_index = false;

	bool raw_in = (reading->tags.find(ztag) != reading->tags.end());

	const Tag *tag = grammar->single_tags.find(ztag)->second;
	if (!tag->is_special) {
		match = raw_in;
	}
	else if (tag->type & T_VARIABLE) {
		if (variables.find(tag->comparison_hash) == variables.end()) {
			u_fprintf(ux_stderr, "Info: %u failed.\n", tag->comparison_hash);
			match = false;
		}
		else {
			u_fprintf(ux_stderr, "Info: %u matched.\n", tag->comparison_hash);
			match = true;
		}
	}
	else if (tag->type & T_NUMERICAL && !reading->tags_numerical->empty()) {
		match = false;
		uint32HashSet::const_iterator mter;
		for (mter = reading->tags_numerical->begin() ; mter != reading->tags_numerical->end() ; mter++) {
			const Tag *itag = single_tags.find(*mter)->second;
			if (tag->comparison_hash == itag->comparison_hash) {
				if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_EQUALS && tag->comparison_val == itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_LESSTHAN && tag->comparison_val < itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_GREATERTHAN && tag->comparison_val > itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_EQUALS && tag->comparison_val > itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_LESSTHAN) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_GREATERTHAN && tag->comparison_val > itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_EQUALS && tag->comparison_val < itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_GREATERTHAN) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_LESSTHAN && tag->comparison_val < itag->comparison_val) {
					match = true;
				}
				if (match) {
					break;
				}
			}
		}
	}
	else if (tag->regexp && !reading->tags_textual->empty()) {
		uint32HashSet::const_iterator mter;
		for (mter = reading->tags_textual->begin() ; mter != reading->tags_textual->end() ; mter++) {
			// ToDo: Cache regexp and icase hits/misses
			const Tag *itag = single_tags.find(*mter)->second;
			UErrorCode status = U_ZERO_ERROR;
			uregex_setText(tag->regexp, itag->tag, u_strlen(itag->tag), &status);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_setText(MatchSet) returned %s - cannot continue!\n", u_errorName(status));
				CG3Quit(1);
			}
			status = U_ZERO_ERROR;
			match = (uregex_matches(tag->regexp, 0, &status) == TRUE);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_matches(MatchSet) returned %s - cannot continue!\n", u_errorName(status));
				CG3Quit(1);
			}
			if (match) {
				break;
			}
		}
	}
	else if (!raw_in) {
		match = false;
		if (tag->type & T_NEGATIVE) {
			match = true;
		}
	}

	if (tag->type & T_NEGATIVE) {
		match = !match;
	}

	if (match) {
		match_single++;
		if (tag->type & T_FAILFAST) {
			retval = false;
		}
		else {
			retval = true;
		}
		// ToDo: Can tag->type ever not contain T_MAPPING if tag->tag[0] == grammar->mapping_prefix?
		if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
			last_mapping_tag = tag->hash;
		}
	}
	return retval;
}

bool GrammarApplicator::doesSetMatchReading(Reading *reading, const uint32_t set, bool bypass_index) {
	bool retval = false;

	if (reading->possible_sets.find(set) == reading->possible_sets.end()) {
		return false;
	}
	if (reading->hash != 1) {
		if (!bypass_index && __index_matches(&index_reading_yes, reading->hash, set)) { return true; }
		if (__index_matches(&index_reading_no, reading->hash, set)) { return false; }
	}

	cache_miss++;

	clock_t tstamp = 0;
	if (statistics) {
		tstamp = clock();
	}

	stdext::hash_map<uint32_t, Set*>::const_iterator iter = grammar->sets_by_contents.find(set);
	if (iter != grammar->sets_by_contents.end()) {
		const Set *theset = iter->second;
		if (theset->is_unified) {
			unif_mode = true;
		}

		if (theset->match_any) {
			retval = true;
		}
		else if (theset->sets.empty()) {
			uint32HashSet::const_iterator ster;

			for (ster = theset->single_tags.begin() ; ster != theset->single_tags.end() ; ster++) {
				bool match = doesTagMatchReading(reading, *ster, bypass_index);
				if (match) {
					if (unif_mode) {
						if (unif_tags.find(theset->hash) != unif_tags.end() && unif_tags[theset->hash] != *ster) {
							continue;
						}
						unif_tags[theset->hash] = *ster;
					}
					retval = true;
					break;
				}
			}

			if (!retval) {
				for (ster = theset->tags.begin() ; ster != theset->tags.end() ; ster++) {
					bool match = true;
					const CompositeTag *ctag = grammar->tags.find(*ster)->second;

					uint32HashSet::const_iterator cter;
					for (cter = ctag->tags.begin() ; cter != ctag->tags.end() ; cter++) {
						bool inner = doesTagMatchReading(reading, *cter, bypass_index);
						if (!inner) {
							match = false;
							break;
						}
					}
					if (match) {
						if (unif_mode) {
							if (unif_tags.find(theset->hash) != unif_tags.end() && unif_tags[theset->hash] != *ster) {
								last_mapping_tag = 0;
								continue;
							}
							unif_tags[theset->hash] = *ster;
						}
						match_comp++;
						retval = true;
						break;
					} else {
						last_mapping_tag = 0;
					}
				}
			}
		}
		else {
			size_t size = theset->sets.size();
			for (size_t i=0;i<size;i++) {
				bool match = doesSetMatchReading(reading, theset->sets.at(i), bypass_index);
				bool failfast = false;
				while (i < size-1 && theset->set_ops.at(i) != S_OR) {
					switch (theset->set_ops.at(i)) {
						case S_PLUS:
							if (match) {
								match = doesSetMatchReading(reading, theset->sets.at(i+1), bypass_index);
							}
							break;
						case S_FAILFAST:
							if (match) {
								if (doesSetMatchReading(reading, theset->sets.at(i+1), bypass_index)) {
									match = false;
									failfast = true;
								}
							}
							break;
						case S_MINUS:
							if (match) {
								if (doesSetMatchReading(reading, theset->sets.at(i+1), bypass_index)) {
									match = false;
								}
							}
							break;
						case S_NOT:
							if (!match) {
								if (!doesSetMatchReading(reading, theset->sets.at(i+1), bypass_index)) {
									match = true;
								}
							}
							break;
						default:
							break;
					}
					i++;
				}
				if (match) {
					match_sub++;
					retval = true;
					break;
				}
				if (failfast) {
					match_sub++;
					retval = false;
					break;
				}
			}
		}
		if (statistics) {
			if (retval) {
				theset->num_match++;
			}
			else {
				theset->num_fail++;
			}
			theset->total_time += clock() - tstamp;
		}
	}

	if (retval) {
		if (reading->hash != 1) {
			if (index_reading_yes.find(reading->hash) == index_reading_yes.end()) {
				Recycler *r = Recycler::instance();
				index_reading_yes[reading->hash] = r->new_uint32HashSet();
			}
			index_reading_yes[reading->hash]->insert(set);
		}
	}
	else {
		if (reading->hash != 1 && !unif_mode) {
			if (index_reading_no.find(reading->hash) == index_reading_no.end()) {
				Recycler *r = Recycler::instance();
				index_reading_no[reading->hash] = r->new_uint32HashSet();
			}
			index_reading_no[reading->hash]->insert(set);
		}
	}

	return retval;
}

bool GrammarApplicator::doesSetMatchCohortNormal(const Cohort *cohort, const uint32_t set) {
	bool retval = false;
	const Set *theset = grammar->sets_by_contents.find(set)->second;
	std::list<Reading*>::const_iterator iter;
	for (iter = cohort->readings.begin() ; iter != cohort->readings.end() ; iter++) {
		Reading *reading = *iter;
		if (doesSetMatchReading(reading, set, theset->has_mappings|theset->is_child_unified)) {
			retval = true;
			break;
		}
	}
	return retval;
}

bool GrammarApplicator::doesSetMatchCohortCareful(const Cohort *cohort, const uint32_t set) {
	bool retval = true;
	const Set *theset = grammar->sets_by_contents.find(set)->second;
	std::list<Reading*>::const_iterator iter;
	for (iter = cohort->readings.begin() ; iter != cohort->readings.end() ; iter++) {
		Reading *reading = *iter;
		last_mapping_tag = 0;
		if (!doesSetMatchReading(reading, set, theset->has_mappings|theset->is_child_unified)) {
			retval = false;
			break;
		}
		// A mapped tag must be the only mapped tag in the reading to be considered a Careful match
		if (last_mapping_tag && reading->tags_mapped->size() > 1) {
			retval = false;
			break;
		}
	}
	return retval;
}

Cohort *GrammarApplicator::doesSetMatchDependency(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test) {
	Cohort *rv = 0;

	bool retval = false;
	if (test->pos & POS_DEP_PARENT && current->dep_self != current->dep_parent) {
		if (sWindow->parent->cohort_map.find(current->dep_parent) == sWindow->parent->cohort_map.end()) {
			u_fprintf(ux_stderr, "Warning: Parent dependency %u -> %u does not exist - ignoring.\n", current->dep_self, current->dep_parent);
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
			if (test->pos & POS_CAREFUL) {
				retval = doesSetMatchCohortCareful(cohort, test->target);
			}
			else {
				retval = doesSetMatchCohortNormal(cohort, test->target);
			}
		}
		if (retval) {
			rv = cohort;
		}
	}
	else {
		const uint32HashSet *deps = 0;
		if (test->pos & POS_DEP_CHILD) {
			deps = &current->dep_children;
		}
		else {
			deps = &current->dep_siblings;
		}

		uint32HashSet::const_iterator dter;
		for (dter = deps->begin() ; dter != deps->end() ; dter++) {
			if (sWindow->parent->cohort_map.find(*dter) == sWindow->parent->cohort_map.end()) {
				if (test->pos & POS_DEP_CHILD) {
					u_fprintf(ux_stderr, "Warning: Child dependency %u -> %u does not exist - ignoring.\n", current->dep_self, *dter);
				}
				else {
					u_fprintf(ux_stderr, "Warning: Sibling dependency %u -> %u does not exist - ignoring.\n", current->dep_self, *dter);
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
				if (test->pos & POS_CAREFUL) {
					retval = doesSetMatchCohortCareful(cohort, test->target);
				}
				else {
					retval = doesSetMatchCohortNormal(cohort, test->target);
				}
			}
			if (retval) {
				rv = cohort;
				break;
			}
		}
	}

	return rv;
}
