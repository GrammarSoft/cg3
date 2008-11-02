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
using namespace CG3::Strings;

inline bool uint32HashSet_Intersects(const uint32HashSet *a, const uint32HashSet *b) {
	const uint32HashSet *outer, *inner;
	if (a->size() > b->size()) {
		inner = a;
		outer = b;
	}
	else {
		inner = b;
		outer = a;
	}
	const_foreach(uint32HashSet, (*outer), oter, oter_end) {
		if (inner->find(*oter) != inner->end()) {
			return true;
		}
	}
	return false;
}

inline bool uint32HashSet_SubsetOf(const uint32HashSet *a, const uint32HashSet *b) {
	const_foreach(uint32HashSet, (*a), oter, oter_end) {
		if (b->find(*oter) == b->end()) {
			return true;
		}
	}
	return false;
}

bool GrammarApplicator::doesTagMatchSet(const uint32_t tag, const Set *set) {
	bool retval = false;
	
	stdext::hash_map<uint32_t, Tag*>::const_iterator itag = grammar->single_tags.find(tag);
	if (itag == grammar->single_tags.end()) {
		return false;
	}

	Tag *t = itag->second;
	if (set->single_tags.find(t) != set->single_tags.end()) {
		retval = true;
	}
	else {
		CompositeTag *ctag = new CompositeTag();
		ctag->addTag(t);
		ctag->rehash();

		if (set->tags.find(ctag) != set->tags.end()) {
			retval = true;
		}
		delete ctag;
	}
	return retval;
}

bool GrammarApplicator::doesTagMatchReading(const Reading *reading, const Tag *tag) {
	bool retval = false;
	bool match = true;

	bool raw_in = (reading->tags_plain.find(tag->hash) != reading->tags_plain.end());

	if (!tag->is_special) {
		match = raw_in;
	}
	else if (tag->type & T_NUMERICAL && !reading->tags_numerical.empty()) {
		match = false;
		uint32HashSet::const_iterator mter;
		for (mter = reading->tags_numerical.begin() ; mter != reading->tags_numerical.end() ; mter++) {
			const Tag *itag = single_tags.find(*mter)->second;
			if (tag->comparison_hash == itag->comparison_hash) {
				if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_EQUALS && tag->comparison_val == itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_NOTEQUALS && itag->comparison_op == OP_EQUALS && tag->comparison_val != itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_NOTEQUALS && tag->comparison_val != itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_NOTEQUALS && itag->comparison_op == OP_NOTEQUALS && tag->comparison_val == itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_LESSTHAN && tag->comparison_val < itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_LESSEQUALS && tag->comparison_val <= itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_GREATERTHAN && tag->comparison_val > itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_EQUALS && itag->comparison_op == OP_GREATEREQUALS && tag->comparison_val >= itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_NOTEQUALS && itag->comparison_op == OP_LESSTHAN) {
					match = true;
				}
				else if (tag->comparison_op == OP_NOTEQUALS && itag->comparison_op == OP_LESSEQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_NOTEQUALS && itag->comparison_op == OP_GREATERTHAN) {
					match = true;
				}
				else if (tag->comparison_op == OP_NOTEQUALS && itag->comparison_op == OP_GREATEREQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_NOTEQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSEQUALS && itag->comparison_op == OP_NOTEQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_NOTEQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATEREQUALS && itag->comparison_op == OP_NOTEQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_EQUALS && tag->comparison_val > itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSEQUALS && itag->comparison_op == OP_EQUALS && tag->comparison_val >= itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_LESSTHAN) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSEQUALS && itag->comparison_op == OP_LESSEQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSEQUALS && itag->comparison_op == OP_LESSTHAN) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_LESSEQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_GREATERTHAN && tag->comparison_val > itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSTHAN && itag->comparison_op == OP_GREATEREQUALS && tag->comparison_val > itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSEQUALS && itag->comparison_op == OP_GREATERTHAN && tag->comparison_val > itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_LESSEQUALS && itag->comparison_op == OP_GREATEREQUALS && tag->comparison_val >= itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_EQUALS && tag->comparison_val < itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATEREQUALS && itag->comparison_op == OP_EQUALS && tag->comparison_val <= itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_GREATERTHAN) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATEREQUALS && itag->comparison_op == OP_GREATEREQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATEREQUALS && itag->comparison_op == OP_GREATERTHAN) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_GREATEREQUALS) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_LESSTHAN && tag->comparison_val < itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATERTHAN && itag->comparison_op == OP_LESSEQUALS && tag->comparison_val < itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATEREQUALS && itag->comparison_op == OP_LESSTHAN && tag->comparison_val < itag->comparison_val) {
					match = true;
				}
				else if (tag->comparison_op == OP_GREATEREQUALS && itag->comparison_op == OP_LESSEQUALS && tag->comparison_val <= itag->comparison_val) {
					match = true;
				}
				if (match) {
					break;
				}
			}
		}
	}
	else if (tag->type & T_REGEXP_ANY) {
		if (tag->type & T_WORDFORM) {
			match = true;
			if (unif_mode) {
				if (unif_last_wordform) {
					if (unif_last_wordform != reading->wordform) {
						match = false;
					}
				}
				else {
					unif_last_wordform = reading->wordform;
				}
			}
		}
		else if (tag->type & T_BASEFORM) {
			match = true;
			if (unif_mode) {
				if (unif_last_baseform) {
					if (unif_last_baseform != reading->baseform) {
						match = false;
					}
				}
				else {
					unif_last_baseform = reading->baseform;
				}
			}
		}
		else {
			const_foreach(uint32HashSet, reading->tags_textual, mter, mter_end) {
				const Tag *itag = single_tags.find(*mter)->second;
				if (!(itag->type & (T_BASEFORM|T_WORDFORM))) {
					match = true;
					if (unif_mode) {
						if (unif_last_textual) {
							if (unif_last_textual != *mter) {
								match = false;
							}
						}
						else {
							unif_last_textual = *mter;
						}
					}
				}
				if (match) {
					break;
				}
			}
		}
	}
	else if (tag->regexp && !reading->tags_textual.empty()) {
		const_foreach(uint32HashSet, reading->tags_textual, mter, mter_end) {
			uint32_t ih = hash_sdbm_uint32_t(tag->hash, *mter);
			if (__index_matches(index_regexp_yes, ih)) {
				match = true;
			}
			else if (__index_matches(index_regexp_no, ih)) {
				match = false;
			}
			else {
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
					index_regexp_yes.insert(ih);
				}
				else {
					index_regexp_no.insert(ih);
				}
			}
			if (match) {
				break;
			}
		}
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
	else if (par_left_tag && tag->type & T_PAR_LEFT && reading->parent->local_number == par_left_pos) {
		match = (reading->tags.find(par_left_tag) != reading->tags.end());
	}
	else if (par_right_tag && tag->type & T_PAR_RIGHT && reading->parent->local_number == par_right_pos) {
		match = (reading->tags.find(par_right_tag) != reading->tags.end());
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
	}
	return retval;
}

bool GrammarApplicator::doesSetMatchReading_tags(const Reading *reading, const Set *theset) {
	bool retval = false;

	if (!(theset->is_special|unif_mode)) {
		retval = uint32HashSet_Intersects(&theset->single_tags_hash, &reading->tags_plain);
	}
	else {
		TagHashSet::const_iterator ster;
		for (ster = theset->single_tags.begin() ; ster != theset->single_tags.end() ; ster++) {
			bool match = doesTagMatchReading(reading, (*ster));
			if (match) {
				if (unif_mode) {
					if (unif_tags.find(theset->hash) != unif_tags.end() && unif_tags[theset->hash] != (*ster)->hash) {
						continue;
					}
					unif_tags[theset->hash] = (*ster)->hash;
				}
				retval = true;
				break;
			}
		}
	}

	if (!retval && !theset->tags.empty()) {
		CompositeTagHashSet::const_iterator ster;
		for (ster = theset->tags.begin() ; ster != theset->tags.end() ; ster++) {
			bool match = true;
			const CompositeTag *ctag = *ster;

			if (!(ctag->is_special|unif_mode)) {
				match = !uint32HashSet_SubsetOf(&ctag->tags_hash, &reading->tags_plain);
			}
			else {
				TagHashSet::const_iterator cter;
				for (cter = ctag->tags.begin() ; cter != ctag->tags.end() ; cter++) {
					bool inner = doesTagMatchReading(reading, (*cter));
					if (!inner) {
						match = false;
						break;
					}
				}
			}
			if (match) {
				if (unif_mode) {
					if (unif_tags.find(theset->hash) != unif_tags.end() && unif_tags[theset->hash] != (*ster)->hash) {
						continue;
					}
					unif_tags[theset->hash] = (*ster)->hash;
				}
				match_comp++;
				retval = true;
				break;
			}
		}
	}

	return retval;
}

bool GrammarApplicator::doesSetMatchReading(Reading *reading, const uint32_t set, bool bypass_index) {
	if (reading->possible_sets.find(set) == reading->possible_sets.end()) {
		return false;
	}
	// ToDo: This is not good enough...while numeric tags are special, their failures can be indexed.
	uint32_t ih = hash_sdbm_uint32_t(reading->hash, set);
	if (!bypass_index && __index_matches(index_reading_no, ih)) {
		return false;
	}
	if (!bypass_index && __index_matches(index_reading_yes, ih)) {
		return true;
	}

	bool retval = false;

	ticks tstamp(gtimer);
	if (statistics) {
		tstamp = getticks();
	}

	Setuint32HashMap::const_iterator iter = grammar->sets_by_contents.find(set);
	const Set *theset = iter->second;
	if (theset->is_unified) {
		unif_mode = true;
	}

	if (theset->match_any) {
		retval = true;
	}
	else if (theset->sets.empty()) {
		retval = doesSetMatchReading_tags(reading, theset);
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
		ticks tmp = getticks();
		theset->total_time += elapsed(tmp, tstamp);
	}

	if (retval) {
		index_reading_yes.insert(ih);
	}
	else {
		if (!unif_mode) {
			index_reading_no.insert(ih);
			/* This actually slows down the overall processing. Removing on cohort-level only is most efficient.
			if (!grammar->sets_any || grammar->sets_any->find(set) == grammar->sets_any->end()) {
				reading->possible_sets.erase(set);
			}
			//*/
		}
	}

	return retval;
}

bool GrammarApplicator::doesSetMatchCohortNormal(Cohort *cohort, const uint32_t set) {
	if (cohort->possible_sets.find(set) == cohort->possible_sets.end()) {
		return false;
	}
	bool retval = false;
	const Set *theset = grammar->sets_by_contents.find(set)->second;
	const_foreach(std::list<Reading*>, cohort->readings, iter, iter_end) {
		Reading *reading = *iter;
		if (doesSetMatchReading(reading, set, theset->is_child_unified|theset->is_special)) {
			retval = true;
			break;
		}
	}
	if (!retval) {
		if (!grammar->sets_any || grammar->sets_any->find(set) == grammar->sets_any->end()) {
			cohort->possible_sets.erase(set);
		}
	}
	return retval;
}

bool GrammarApplicator::doesSetMatchCohortCareful(const Cohort *cohort, const uint32_t set) {
	if (cohort->possible_sets.find(set) == cohort->possible_sets.end()) {
		return false;
	}
	bool retval = true;
	const Set *theset = grammar->sets_by_contents.find(set)->second;
	const_foreach(std::list<Reading*>, cohort->readings, iter, iter_end) {
		Reading *reading = *iter;
		if (!doesSetMatchReading(reading, set, theset->is_child_unified|theset->is_special)) {
			retval = false;
			break;
		}
	}
	return retval;
}
