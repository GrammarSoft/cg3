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

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"
#include "ContextualTest.hpp"

namespace CG3 {

/**
 * Tests whether one set is a subset of another set, specialized for TagSet.
 *
 * In the https://visl.sdu.dk/cg3_performance.html test data, this function is executed 1516098 times,
 * of which 23222 (1.5%) return true.
 *
 * @param[in] a The tags from the set
 * @param[in] b The tags from the reading
 */
template<typename T>
inline bool TagSet_SubsetOf_TSet(const TagSortedVector& a, const T& b) {
	/* This test is true 0.1% of the time. Not worth the trouble.
	if (a.size() > b.size()) {
		return false;
	}
	//*/
	typename T::const_iterator bi = b.lower_bound((*a.begin())->hash);
	for (auto ai : a) {
		while (bi != b.end() && *bi < ai->hash) {
			++bi;
		}
		if (bi == b.end() || *bi != ai->hash) {
			return false;
		}
	}
	return true;
}

/**
* Tests whether a given input tag matches a given tag's stored regular expression.
*
* @param[in] test The tag to be tested
* @param[in] tag The tag to test against; only uses the hash and regexp members
*/
uint32_t GrammarApplicator::doesTagMatchRegexp(uint32_t test, const Tag& tag, bool bypass_index) {
	uint32_t match = 0;
	uint32_t ih = hash_value(tag.hash, test);
	if (!bypass_index && index_matches(index_regexp_no, ih)) {
		match = 0;
	}
	else if (!bypass_index && index_matches(index_regexp_yes, ih)) {
		match = test;
	}
	else {
		const Tag& itag = *(single_tags.find(test)->second);
		UErrorCode status = U_ZERO_ERROR;
		uregex_setText(tag.regexp, itag.tag.c_str(), itag.tag.size(), &status);
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: uregex_setText(MatchSet) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.c_str(), numLines);
			CG3Quit(1);
		}
		status = U_ZERO_ERROR;
		if (uregex_find(tag.regexp, -1, &status)) {
			match = itag.hash;
		}
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: uregex_find(MatchSet) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.c_str(), numLines);
			CG3Quit(1);
		}
		if (match) {
			int32_t gc = uregex_groupCount(tag.regexp, &status);
			// ToDo: Allow regex captures from dependency target contexts without any captures in normal target contexts
			if (gc > 0 && regexgrps.second != 0) {
				UChar tmp[1024];
				for (int i = 1; i <= gc; ++i) {
					tmp[0] = 0;
					int32_t len = uregex_group(tag.regexp, i, tmp, 1024, &status);
					regexgrps.second->resize(std::max(static_cast<size_t>(regexgrps.first) + 1, regexgrps.second->size()));
					UnicodeString& ucstr = (*regexgrps.second)[regexgrps.first];
					ucstr.remove();
					ucstr.append(tmp, len);
					++regexgrps.first;
				}
			}
			else {
				index_regexp_yes.insert(ih);
			}
		}
		else {
			index_regexp_no.insert(ih);
		}
	}
	return match;
}

uint32_t GrammarApplicator::doesTagMatchIcase(uint32_t test, const Tag& tag, bool bypass_index) {
	uint32_t match = 0;
	uint32_t ih = hash_value(tag.hash, test);
	if (!bypass_index && index_matches(index_icase_no, ih)) {
		match = 0;
	}
	else if (!bypass_index && index_matches(index_icase_yes, ih)) {
		match = test;
	}
	else {
		const Tag& itag = *(single_tags.find(test)->second);
		UErrorCode status = U_ZERO_ERROR;
		if (u_strCaseCompare(tag.tag.c_str(), tag.tag.size(), itag.tag.c_str(), itag.tag.size(), U_FOLD_CASE_DEFAULT, &status) == 0) {
			match = itag.hash;
		}
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: u_strCaseCompare() returned %s - cannot continue!\n", u_errorName(status));
			CG3Quit(1);
		}
		if (match) {
			index_icase_yes.insert(ih);
		}
		else {
			index_icase_no.insert(ih);
		}
	}
	return match;
}

// ToDo: Remove for real ordered mode
uint32_t GrammarApplicator::doesRegexpMatchLine(const Reading& reading, const Tag& tag, bool bypass_index) {
	uint32_t match = 0;
	uint32_t ih = hash_value(reading.tags_string_hash, tag.hash);
	if (!bypass_index && index_matches(index_regexp_no, ih)) {
		match = 0;
	}
	else if (!bypass_index && index_matches(index_regexp_yes, ih)) {
		match = reading.tags_string_hash;
	}
	else {
		UErrorCode status = U_ZERO_ERROR;
		uregex_setText(tag.regexp, reading.tags_string.c_str(), reading.tags_string.size(), &status);
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: uregex_setText(MatchSet) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.c_str(), numLines);
			CG3Quit(1);
		}
		status = U_ZERO_ERROR;
		if (uregex_find(tag.regexp, -1, &status)) {
			match = reading.tags_string_hash;
		}
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: uregex_find(MatchSet) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.c_str(), numLines);
			CG3Quit(1);
		}
		if (match) {
			int32_t gc = uregex_groupCount(tag.regexp, &status);
			// ToDo: Allow regex captures from dependency target contexts without any captures in normal target contexts
			if (gc > 0 && regexgrps.second != 0) {
				UChar tmp[1024];
				for (int i = 1; i <= gc; ++i) {
					tmp[0] = 0;
					int32_t len = uregex_group(tag.regexp, i, tmp, 1024, &status);
					regexgrps.second->resize(std::max(static_cast<size_t>(regexgrps.first) + 1, regexgrps.second->size()));
					UnicodeString& ucstr = (*regexgrps.second)[regexgrps.first];
					ucstr.remove();
					ucstr.append(tmp, len);
					++regexgrps.first;
				}
			}
			else {
				index_regexp_yes.insert(ih);
			}
		}
		else {
			index_regexp_no.insert(ih);
		}
	}
	return match;
}

/**
 * Tests whether a given reading matches a given tag's stored regular expression.
 *
 * @param[in] reading The reading to test
 * @param[in] tag The tag to test against; only uses the hash and regexp members
 */
uint32_t GrammarApplicator::doesRegexpMatchReading(const Reading& reading, const Tag& tag, bool bypass_index) {
	uint32_t match = 0;

	// ToDo: Remove for real ordered mode
	if (tag.type & T_REGEXP_LINE) {
		return doesRegexpMatchLine(reading, tag, bypass_index);
	}

	// Grammar::reindex() will do a one-time pass to mark any potential matching tag as T_TEXTUAL
	for (auto mter : reading.tags_textual) {
		match = doesTagMatchRegexp(mter, tag, bypass_index);
		if (match) {
			break;
		}
	}

	return match;
}

/**
 * Tests whether a given reading matches a given tag.
 *
 * In the https://visl.sdu.dk/cg3_performance.html test data, this function is executed 1058428 times,
 * of which 827259 are treated as raw tags.
 *
 * @param[in] reading The reading to test
 * @param[in] tag The tag to test against
 * @param[in] unif_mode Used to signal that a parent set was a $$unified set
 */
uint32_t GrammarApplicator::doesTagMatchReading(const Reading& reading, const Tag& tag, bool unif_mode, bool bypass_index) {
	uint32_t retval = 0;
	uint32_t match = 0;

	if (!(tag.type & T_SPECIAL) || tag.type & T_FAILFAST) {
		uint32SortedVector::const_iterator itf, ite = reading.tags_plain.end();
		bool raw_in = reading.tags_plain_bloom.matches(tag.hash);
		if (tag.type & T_FAILFAST) {
			itf = reading.tags_plain.find(tag.plain_hash);
			raw_in = (itf != ite);
		}
		else if (raw_in) {
			itf = reading.tags_plain.find(tag.hash);
			raw_in = (itf != ite);
		}
		if (raw_in) {
			match = tag.hash;
		}
	}
	else if (tag.type & T_SET) {
		uint32_t sh = hash_value(tag.tag);
		sh = grammar->sets_by_name.find(sh)->second;
		match = doesSetMatchReading(reading, sh, bypass_index, unif_mode);
	}
	else if (tag.type & T_VARSTRING) {
		const Tag* nt = generateVarstringTag(&tag);
		match = doesTagMatchReading(reading, *nt, unif_mode, bypass_index);
	}
	else if (tag.type & T_META) {
		if (tag.regexp && !reading.parent->text.empty()) {
			UErrorCode status = U_ZERO_ERROR;
			uregex_setText(tag.regexp, reading.parent->text.c_str(), reading.parent->text.size(), &status);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_setText(MatchSet) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.c_str(), numLines);
				CG3Quit(1);
			}
			status = U_ZERO_ERROR;
			if (uregex_find(tag.regexp, -1, &status)) {
				match = tag.hash;
			}
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_find(MatchSet) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.c_str(), numLines);
				CG3Quit(1);
			}
			if (match) {
				int32_t gc = uregex_groupCount(tag.regexp, &status);
				if (gc > 0 && regexgrps.second != 0) {
					UChar tmp[1024];
					for (int i = 1; i <= gc; ++i) {
						tmp[0] = 0;
						int32_t len = uregex_group(tag.regexp, i, tmp, 1024, &status);
						regexgrps.second->resize(std::max(static_cast<size_t>(regexgrps.first) + 1, regexgrps.second->size()));
						UnicodeString& ucstr = (*regexgrps.second)[regexgrps.first];
						ucstr.remove();
						ucstr.append(tmp, len);
						++regexgrps.first;
					}
				}
			}
		}
	}
	else if (tag.regexp) {
		match = doesRegexpMatchReading(reading, tag, bypass_index);
	}
	else if (tag.type & T_CASE_INSENSITIVE) {
		for (auto mter : reading.tags_textual) {
			match = doesTagMatchIcase(mter, tag, bypass_index);
			if (match) {
				break;
			}
		}
	}
	else if (tag.type & T_REGEXP_ANY) {
		if (tag.type & T_BASEFORM) {
			match = reading.baseform;
			if (unif_mode) {
				if (unif_last_baseform) {
					if (unif_last_baseform != reading.baseform) {
						match = 0;
					}
				}
				else {
					unif_last_baseform = reading.baseform;
				}
			}
		}
		else if (tag.type & T_WORDFORM) {
			match = reading.parent->wordform->hash;
			if (unif_mode) {
				if (unif_last_wordform) {
					if (unif_last_wordform != reading.parent->wordform->hash) {
						match = 0;
					}
				}
				else {
					unif_last_wordform = reading.parent->wordform->hash;
				}
			}
		}
		else {
			for (auto mter : reading.tags_textual) {
				const Tag& itag = *(single_tags.find(mter)->second);
				if (!(itag.type & (T_BASEFORM | T_WORDFORM))) {
					match = itag.hash;
					if (unif_mode) {
						if (unif_last_textual) {
							if (unif_last_textual != mter) {
								match = 0;
							}
						}
						else {
							unif_last_textual = mter;
						}
					}
				}
				if (match) {
					break;
				}
			}
		}
	}
	else if (tag.type & T_NUMERICAL) {
		for (auto mter : reading.tags_numerical) {
			const Tag& itag = *(mter.second);
			double compval = tag.comparison_val;
			if (compval <= NUMERIC_MIN) {
				compval = reading.parent->getMin(tag.comparison_hash);
			}
			else if (compval >= NUMERIC_MAX) {
				compval = reading.parent->getMax(tag.comparison_hash);
			}
			if (tag.comparison_hash == itag.comparison_hash) {
				if (tag.comparison_op == OP_EQUALS && itag.comparison_op == OP_EQUALS && compval == itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_NOTEQUALS && itag.comparison_op == OP_EQUALS && compval != itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_EQUALS && itag.comparison_op == OP_NOTEQUALS && compval != itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_NOTEQUALS && itag.comparison_op == OP_NOTEQUALS && compval == itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_EQUALS && itag.comparison_op == OP_LESSTHAN && compval < itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_EQUALS && itag.comparison_op == OP_LESSEQUALS && compval <= itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_EQUALS && itag.comparison_op == OP_GREATERTHAN && compval > itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_EQUALS && itag.comparison_op == OP_GREATEREQUALS && compval >= itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_NOTEQUALS && itag.comparison_op == OP_LESSTHAN) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_NOTEQUALS && itag.comparison_op == OP_LESSEQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_NOTEQUALS && itag.comparison_op == OP_GREATERTHAN) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_NOTEQUALS && itag.comparison_op == OP_GREATEREQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSTHAN && itag.comparison_op == OP_NOTEQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSEQUALS && itag.comparison_op == OP_NOTEQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATERTHAN && itag.comparison_op == OP_NOTEQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATEREQUALS && itag.comparison_op == OP_NOTEQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSTHAN && itag.comparison_op == OP_EQUALS && compval > itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSEQUALS && itag.comparison_op == OP_EQUALS && compval >= itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSTHAN && itag.comparison_op == OP_LESSTHAN) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSEQUALS && itag.comparison_op == OP_LESSEQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSEQUALS && itag.comparison_op == OP_LESSTHAN) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSTHAN && itag.comparison_op == OP_LESSEQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSTHAN && itag.comparison_op == OP_GREATERTHAN && compval > itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSTHAN && itag.comparison_op == OP_GREATEREQUALS && compval > itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSEQUALS && itag.comparison_op == OP_GREATERTHAN && compval > itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_LESSEQUALS && itag.comparison_op == OP_GREATEREQUALS && compval >= itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATERTHAN && itag.comparison_op == OP_EQUALS && compval < itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATEREQUALS && itag.comparison_op == OP_EQUALS && compval <= itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATERTHAN && itag.comparison_op == OP_GREATERTHAN) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATEREQUALS && itag.comparison_op == OP_GREATEREQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATEREQUALS && itag.comparison_op == OP_GREATERTHAN) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATERTHAN && itag.comparison_op == OP_GREATEREQUALS) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATERTHAN && itag.comparison_op == OP_LESSTHAN && compval < itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATERTHAN && itag.comparison_op == OP_LESSEQUALS && compval < itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATEREQUALS && itag.comparison_op == OP_LESSTHAN && compval < itag.comparison_val) {
					match = itag.hash;
				}
				else if (tag.comparison_op == OP_GREATEREQUALS && itag.comparison_op == OP_LESSEQUALS && compval <= itag.comparison_val) {
					match = itag.hash;
				}
				if (match) {
					break;
				}
			}
		}
	}
	else if (tag.type & T_VARIABLE) {
		if (variables.find(tag.comparison_hash) == variables.end()) {
			//u_fprintf(ux_stderr, "Info: %S failed.\n", tag.tag.c_str());
			match = 0;
		}
		else {
			//u_fprintf(ux_stderr, "Info: %S matched.\n", tag.tag.c_str());
			match = tag.hash;
		}
	}
	else if (tag.type & T_PAR_LEFT) {
		if (par_left_tag && reading.parent->local_number == par_left_pos && reading.tags.find(par_left_tag) != reading.tags.end()) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_PAR_RIGHT) {
		if (par_right_tag && reading.parent->local_number == par_right_pos && reading.tags.find(par_right_tag) != reading.tags.end()) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_ENCL) {
		if (!reading.parent->enclosed.empty()) {
			match = true;
		}
	}
	else if (tag.type & T_TARGET) {
		if (target && reading.parent == target) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_MARK) {
		if (mark && reading.parent == mark) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_ATTACHTO) {
		if (attach_to && reading.parent == attach_to) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_SAME_BASIC) {
		if (reading.hash_plain == same_basic) {
			match = grammar->tag_any;
		}
	}

	if (match) {
		++match_single;
		retval = match;
	}

	return retval;
}

bool GrammarApplicator::doesSetMatchReading_trie(const Reading& reading, const Set& theset, const trie_t& trie, bool unif_mode) {
	for (auto& kv : trie) {
		bool match = (doesTagMatchReading(reading, *kv.first, unif_mode) != 0);
		if (match) {
			if (kv.first->type & T_FAILFAST) {
				continue;
			}
			if (kv.second.terminal) {
				if (unif_mode) {
					auto it = unif_tags->find(theset.number);
					if (it != unif_tags->end() && it->second != &kv) {
						continue;
					}
					(*unif_tags)[theset.number] = &kv;
				}
				return true;
			}
			if (kv.second.trie && doesSetMatchReading_trie(reading, theset, *kv.second.trie, unif_mode)) {
				return true;
			}
		}
	}
	return false;
}

/**
 * Tests whether a given reading matches a given LIST set.
 *
 * In the https://visl.sdu.dk/cg3_performance.html test data, this function is executed 1073969 times.
 *
 * @param[in] reading The reading to test
 * @param[in] set The hash of the set to test against
 * @param[in] unif_mode Used to signal that a parent set was a $$unified set
 */
bool GrammarApplicator::doesSetMatchReading_tags(const Reading& reading, const Set& theset, bool unif_mode) {
	bool retval = false;

	if (!theset.ff_tags.empty()) {
		for (auto tag : theset.ff_tags) {
			if (doesTagMatchReading(reading, *tag, unif_mode)) {
				return false;
			}
		}
	}

	// If there are no special circumstances the first test boils down to finding whether the tag stores intersect
	// 80% of calls try this first.
	if (!theset.trie.empty() && !reading.tags_plain.empty()) {
		trie_t::const_iterator iiter = theset.trie.lower_bound(single_tags.find(reading.tags_plain.front())->second);
		uint32SortedVector::const_iterator oiter = reading.tags_plain.lower_bound(theset.trie.begin()->first->hash);
		while (oiter != reading.tags_plain.end() && iiter != theset.trie.end()) {
			if (*oiter == iiter->first->hash) {
				if (iiter->second.terminal) {
					if (unif_mode) {
						auto it = unif_tags->find(theset.number);
						if (it != unif_tags->end() && it->second != &*iiter) {
							++iiter;
							continue;
						}
						(*unif_tags)[theset.number] = &*iiter;
					}
					retval = true;
					break;
				}
				if (iiter->second.trie && doesSetMatchReading_trie(reading, theset, *iiter->second.trie, unif_mode)) {
					retval = true;
					break;
				}
				++iiter;
			}
			while (oiter != reading.tags_plain.end() && iiter != theset.trie.end() && *oiter < iiter->first->hash) {
				++oiter;
			}
			while (oiter != reading.tags_plain.end() && iiter != theset.trie.end() && iiter->first->hash < *oiter) {
				++iiter;
			}
		}
	}

	if (!retval && !theset.trie_special.empty()) {
		retval = doesSetMatchReading_trie(reading, theset, theset.trie_special, unif_mode);
	}

	return retval;
}

/**
 * Tests whether a given reading matches a given LIST or SET set.
 *
 * In the https://visl.sdu.dk/cg3_performance.html test data, this function is executed 5746792 times,
 * of which only 1700292 make it past the first index check.
 *
 * @param[in] reading The reading to test
 * @param[in] set The hash of the set to test against
 * @param[in] bypass_index Flag whether to bypass indexes; needed for certain tests, such as unification and sets with special tags
 * @param[in] unif_mode Used to signal that a parent set was a $$unified set
 */
bool GrammarApplicator::doesSetMatchReading(const Reading& reading, const uint32_t set, bool bypass_index, bool unif_mode) {
	// Check whether we have previously seen that this set matches or doesn't match this reading.
	// These indexes are cleared every ((num_windows+4)*2+1) windows to avoid memory ballooning.
	// Only 30% of tests get past this.
	// ToDo: This is not good enough...while numeric tags are special, their failures can be indexed.
	if (!bypass_index && !unif_mode) {
		if (index_readingSet_no[set].find(reading.hash) != index_readingSet_no[set].end()) {
			return false;
		}
		if (index_readingSet_yes[set].find(reading.hash) != index_readingSet_yes[set].end()) {
			return true;
		}
	}

	bool retval = false;

	ticks tstamp(gtimer);
	if (statistics) {
		tstamp = getticks();
	}

	// ToDo: Make all places have Set* directly so we don't need to perform this lookup.
	const Set& theset = *grammar->sets_list[set];

	// The (*) set always matches.
	if (theset.type & ST_ANY) {
		retval = true;
	}
	// If there are no sub-sets, it must be a LIST set.
	else if (theset.sets.empty()) {
		retval = doesSetMatchReading_tags(reading, theset, ((theset.type & ST_TAG_UNIFY) != 0) | unif_mode);
	}
	// &&unified sets
	else if (theset.type & ST_SET_UNIFY) {
		// ToDo: Handle multiple active &&sets at a time.
		// First time, figure out all the sub-sets that match the reading and store them for later comparison
		if (unif_sets_firstrun) {
			const Set& uset = *grammar->sets_list[theset.sets[0]];
			const size_t size = uset.sets.size();
			for (size_t i = 0; i < size; ++i) {
				const Set& tset = *grammar->sets_list[uset.sets[i]];
				if (doesSetMatchReading(reading, tset.number, bypass_index, ((theset.type & ST_TAG_UNIFY) != 0) | unif_mode)) {
					unif_sets->insert(tset.number);
				}
			}
			retval = !unif_sets->empty();
			unif_sets_firstrun = !retval;
		}
		// Subsequent times, test whether any of the previously stored sets match the reading
		else {
			auto sets = ss_u32sv.get();
			for (auto usi : *unif_sets) {
				if (doesSetMatchReading(reading, usi, bypass_index, unif_mode)) {
					sets->insert(usi);
				}
			}
			retval = !sets->empty();
		}
	}
	else {
		// If all else fails, it must be a SET set.
		// Loop through the sub-sets and apply the set operators
		const size_t size = theset.sets.size();
		for (size_t i = 0; i < size; ++i) {
			bool match = doesSetMatchReading(reading, theset.sets[i], bypass_index, ((theset.type & ST_TAG_UNIFY) != 0) | unif_mode);
			bool failfast = false;
			// Operator OR does not modify match, so simply skip it.
			// The result of doing so means that the other operators gain precedence.
			while (i < size - 1 && theset.set_ops[i] != S_OR) {
				switch (theset.set_ops[i]) {
				case S_PLUS:
					if (match) {
						match = doesSetMatchReading(reading, theset.sets[i + 1], bypass_index, ((theset.type & ST_TAG_UNIFY) != 0) | unif_mode);
					}
					break;
				// Failfast makes a difference in A OR B ^ C OR D, where - does not.
				case S_FAILFAST:
					if (doesSetMatchReading(reading, theset.sets[i + 1], bypass_index, ((theset.type & ST_TAG_UNIFY) != 0) | unif_mode)) {
						match = false;
						failfast = true;
					}
					break;
				case S_MINUS:
					if (match) {
						if (doesSetMatchReading(reading, theset.sets[i + 1], bypass_index, ((theset.type & ST_TAG_UNIFY) != 0) | unif_mode)) {
							match = false;
						}
					}
					break;
				case S_NOT:
					if (!match) {
						if (!doesSetMatchReading(reading, theset.sets[i + 1], bypass_index, ((theset.type & ST_TAG_UNIFY) != 0) | unif_mode)) {
							match = true;
						}
					}
					break;
				default:
					break;
				}
				++i;
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
		// Propagate unified tag to other sets of this set, if applicable
		if (unif_mode || (theset.type & ST_TAG_UNIFY)) {
			const void* tag = 0;
			for (size_t i = 0; i < size; ++i) {
				auto it = unif_tags->find(theset.sets[i]);
				if (it != unif_tags->end()) {
					tag = it->second;
					break;
				}
			}
			if (tag) {
				for (size_t i = 0; i < size; ++i) {
					(*unif_tags)[theset.sets[i]] = tag;
				}
			}
		}
	}
	if (statistics) {
		if (retval) {
			theset.num_match++;
		}
		else {
			theset.num_fail++;
		}
		ticks tmp = getticks();
		theset.total_time += elapsed(tmp, tstamp);
	}

	// Store the result in the indexes in hopes that later runs can pull it directly from them.
	if (retval) {
		index_readingSet_yes[set].insert(reading.hash);
	}
	else {
		if (!(theset.type & ST_TAG_UNIFY) && !unif_mode) {
			index_readingSet_no[set].insert(reading.hash);
		}
	}

	return retval;
}

inline bool _check_options(std::vector<Reading*>& rv, uint32_t options, size_t nr) {
	if ((options & POS_CAREFUL) && rv.size() != nr) {
		return false;
	}
	if (options & MASK_POS_DEPREL) {
		return true;
	}
	return !rv.empty();
}

inline bool GrammarApplicator::doesSetMatchCohort_testLinked(Cohort& cohort, const Set& theset, dSMC_Context* context) {
	bool retval = true;
	bool reset = false;
	const ContextualTest* linked = 0;
	Cohort* min = 0;
	Cohort* max = 0;

	if (context->test && context->test->linked) {
		linked = context->test->linked;
	}
	else if (!tmpl_cntx.linked.empty()) {
		min = tmpl_cntx.min;
		max = tmpl_cntx.max;
		linked = tmpl_cntx.linked.back();
		tmpl_cntx.linked.pop_back();
		reset = true;
	}
	if (linked) {
		if (!context->did_test) {
			if (linked->pos & POS_NO_PASS_ORIGIN) {
				context->matched_tests = (runContextualTest(cohort.parent, cohort.local_number, linked, context->deep, &cohort) != 0);
			}
			else {
				context->matched_tests = (runContextualTest(cohort.parent, cohort.local_number, linked, context->deep, context->origin) != 0);
			}
			if (!(theset.type & ST_CHILD_UNIFY)) {
				context->did_test = true;
			}
		}
		retval = context->matched_tests;
	}
	if (reset) {
		tmpl_cntx.linked.push_back(linked);
	}
	if (!retval) {
		tmpl_cntx.min = min;
		tmpl_cntx.max = max;
	}
	return retval;
}

inline bool GrammarApplicator::doesSetMatchCohort_helper(Cohort& cohort, Reading& reading, const Set& theset, dSMC_Context* context) {
	bool retval = false;
	auto utags = ss_utags.get();
	auto usets = ss_u32sv.get();
	uint8_t orz = regexgrps.first;

	if (context && !(current_rule->flags & FL_CAPTURE_UNIF) && (theset.type & ST_CHILD_UNIFY)) {
		*utags = *unif_tags;
		*usets = *unif_sets;
	}
	if (doesSetMatchReading(reading, theset.number, (theset.type & (ST_CHILD_UNIFY | ST_SPECIAL)) != 0)) {
		retval = true;
		if (context) {
			if (context->options & POS_ATTACH_TO) {
				reading.matched_target = true;
			}
			context->matched_target = true;
		}
	}
	if (retval && context && (context->options & POS_NOT)) {
		retval = !retval;
	}
	if (retval && context && !context->in_barrier) {
		retval = doesSetMatchCohort_testLinked(cohort, theset, context);
		if (context->options & POS_ATTACH_TO) {
			reading.matched_tests = retval;
		}
	}
	if (!retval && context && !(current_rule->flags & FL_CAPTURE_UNIF) && (theset.type & ST_CHILD_UNIFY) && (utags->size() != unif_tags->size() || *utags != *unif_tags)) {
		unif_tags->swap(utags);
	}
	if (!retval && context && !(current_rule->flags & FL_CAPTURE_UNIF) && (theset.type & ST_CHILD_UNIFY) && usets->size() != unif_sets->size()) {
		unif_sets->swap(usets);
	}
	if (!retval) {
		regexgrps.first = orz;
	}
	return retval;
}

bool GrammarApplicator::doesSetMatchCohortNormal(Cohort& cohort, const uint32_t set, dSMC_Context* context) {
	bool retval = false;

	if (!(!context || (context->options & (POS_LOOK_DELETED | POS_LOOK_DELAYED | POS_NOT))) && (set >= cohort.possible_sets.size() || !cohort.possible_sets.test(set))) {
		return retval;
	}

	const Set* theset = grammar->sets_list[set];

	if (cohort.wread) {
		retval = doesSetMatchCohort_helper(cohort, *cohort.wread, *theset, context);
	}

	if (retval && (!context || context->did_test)) {
		return retval;
	}

	ReadingList* lists[3] = { &cohort.readings };
	if (context && (context->options & POS_LOOK_DELETED)) {
		lists[1] = &cohort.deleted;
	}
	if (context && (context->options & POS_LOOK_DELAYED)) {
		lists[2] = &cohort.delayed;
	}

	for (size_t i = 0; i < 3; ++i) {
		if (lists[i] == 0) {
			continue;
		}
		for (auto reading : *lists[i]) {
			if (context && context->test) {
				// ToDo: Barriers need some way to escape sub-readings
				reading = get_sub_reading(reading, context->test->offset_sub);
				if (!reading) {
					continue;
				}
			}
			if (doesSetMatchCohort_helper(cohort, *reading, *theset, context)) {
				retval = true;
			}
			if (retval && (!context || !(context->test && context->test->linked) || context->did_test)) {
				return retval;
			}
		}
	}

	if (context && !context->matched_target && (context->options & POS_NOT)) {
		retval = doesSetMatchCohort_testLinked(cohort, *theset, context);
	}

	if (context && !context->matched_target) {
		if (!grammar->sets_any || set >= grammar->sets_any->size() || !grammar->sets_any->test(set)) {
			if (set < cohort.possible_sets.size()) {
				cohort.possible_sets.reset(set);
			}
		}
	}

	return retval;
}

bool GrammarApplicator::doesSetMatchCohortCareful(Cohort& cohort, const uint32_t set, dSMC_Context* context) {
	bool retval = false;

	if (!(!context || (context->options & (POS_LOOK_DELETED | POS_LOOK_DELAYED | POS_NOT))) && (set >= cohort.possible_sets.size() || !cohort.possible_sets.test(set))) {
		return retval;
	}

	const Set* theset = grammar->sets_list[set];

	ReadingList* lists[3] = { &cohort.readings };
	if (context && (context->options & POS_LOOK_DELETED)) {
		lists[1] = &cohort.deleted;
	}
	if (context && (context->options & POS_LOOK_DELAYED)) {
		lists[2] = &cohort.delayed;
	}

	for (size_t i = 0; i < 3; ++i) {
		if (lists[i] == 0) {
			continue;
		}
		for (auto reading : *lists[i]) {
			if (context && context->test) {
				// ToDo: Barriers need some way to escape sub-readings
				reading = get_sub_reading(reading, context->test->offset_sub);
				if (!reading) {
					continue;
				}
			}
			retval = doesSetMatchCohort_helper(cohort, *reading, *theset, context);
			if (!retval) {
				break;
			}
		}
		if (!retval) {
			break;
		}
	}

	if (context && !context->matched_target && (context->options & POS_NOT)) {
		retval = doesSetMatchCohort_testLinked(cohort, *theset, context);
	}

	return retval;
}
}
