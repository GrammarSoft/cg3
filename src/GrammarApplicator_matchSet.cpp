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
#include "MathParser.hpp"

namespace CG3 {

/**
 * Tests whether one set is a subset of another set, specialized for TagSet.
 *
 * In the https://edu.visl.dk/cg3_performance.html test data, this function is executed 1516098 times,
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
	auto bi = b.lower_bound((*a.begin())->hash);
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

template<typename RXGS, typename Tag>
inline void captureRegex(int32_t gc, uint8_t& regexgrp_ct, RXGS* regexgrps, Tag& tag) {
	constexpr auto BUFSIZE = 1024;
	UErrorCode status = U_ZERO_ERROR;
	UChar _tmp[BUFSIZE];
	UString _stmp;
	UChar* tmp = _tmp;
	for (int i = 1; i <= gc; ++i) {
		tmp[0] = 0;
		int32_t len = uregex_group(tag.regexp, i, tmp, BUFSIZE, &status);
		if (len >= BUFSIZE) {
			status = U_ZERO_ERROR;
			_stmp.resize(len + 1);
			tmp = &_stmp[0];
			uregex_group(tag.regexp, i, tmp, len + 1, &status);
		}
		regexgrps->resize(std::max(static_cast<size_t>(regexgrp_ct) + 1, regexgrps->size()));
		UnicodeString& ucstr = (*regexgrps)[regexgrp_ct];
		ucstr.remove();
		ucstr.append(tmp, len);
		++regexgrp_ct;
	}
}

/**
* Tests whether a given input tag matches a given tag's stored regular expression.
*
* @param[in] test The tag to be tested
* @param[in] tag The tag to test against; only uses the hash and regexp members
*/
uint32_t GrammarApplicator::doesTagMatchRegexp(uint32_t test, const Tag& tag, bool bypass_index) {
	UErrorCode status = U_ZERO_ERROR;
	int32_t gc = uregex_groupCount(tag.regexp, &status);
	uint32_t match = 0;
	auto ih = (UI64(tag.hash) << 32) | test;
	if (!bypass_index && index_regexp_no.contains(ih)) {
		match = 0;
	}
	else if (!bypass_index && gc == 0 && index_regexp_yes.contains(ih)) {
		match = test;
	}
	else {
		const Tag& itag = *(grammar->single_tags.find(test)->second);
		uregex_setText(tag.regexp, itag.tag.data(), SI32(itag.tag.size()), &status);
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: uregex_setText(MatchTag) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.data(), numLines);
			CG3Quit(1);
		}
		status = U_ZERO_ERROR;
		if (uregex_find(tag.regexp, -1, &status)) {
			match = itag.hash;
		}
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: uregex_find(MatchTag) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.data(), numLines);
			CG3Quit(1);
		}
		if (match) {
			if (gc > 0 && !context_stack.empty() && context_stack.back().regexgrps != 0) {
				captureRegex(gc, context_stack.back().regexgrp_ct, context_stack.back().regexgrps, tag);
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
	auto ih = (UI64(tag.hash) << 32) | test;
	if (!bypass_index && index_icase_no.contains(ih)) {
		match = 0;
	}
	else if (!bypass_index && index_icase_yes.contains(ih)) {
		match = test;
	}
	else {
		const Tag& itag = *(grammar->single_tags.find(test)->second);
		if (ux_strCaseCompare(tag.tag, itag.tag)) {
			match = itag.hash;
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
	UErrorCode status = U_ZERO_ERROR;
	int32_t gc = uregex_groupCount(tag.regexp, &status);
	uint32_t match = 0;
	auto ih = (UI64(reading.tags_string_hash) << 32) | tag.hash;
	if (!bypass_index && index_regexp_no.contains(ih)) {
		match = 0;
	}
	else if (!bypass_index && gc == 0 && index_regexp_yes.contains(ih)) {
		match = reading.tags_string_hash;
	}
	else {
		uregex_setText(tag.regexp, reading.tags_string.data(), SI32(reading.tags_string.size()), &status);
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: uregex_setText(MatchLine) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.data(), numLines);
			CG3Quit(1);
		}
		status = U_ZERO_ERROR;
		if (uregex_find(tag.regexp, -1, &status)) {
			match = reading.tags_string_hash;
		}
		if (status != U_ZERO_ERROR) {
			u_fprintf(ux_stderr, "Error: uregex_find(MatchLine) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.data(), numLines);
			CG3Quit(1);
		}
		if (match) {
			// ToDo: Allow regex captures from dependency target contexts without any captures in normal target contexts
			if (gc > 0 && !context_stack.empty() && context_stack.back().regexgrps != 0) {
				captureRegex(gc, context_stack.back().regexgrp_ct, context_stack.back().regexgrps, tag);
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
 * In the https://edu.visl.dk/cg3_performance.html test data, this function is executed 1058428 times,
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
			uregex_setText(tag.regexp, reading.parent->text.data(), SI32(reading.parent->text.size()), &status);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_setText(MatchSet) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.data(), numLines);
				CG3Quit(1);
			}
			status = U_ZERO_ERROR;
			if (uregex_find(tag.regexp, -1, &status)) {
				match = tag.hash;
			}
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_find(MatchSet) returned %s for tag %S before input line %u - cannot continue!\n", u_errorName(status), tag.tag.data(), numLines);
				CG3Quit(1);
			}
			if (match) {
				int32_t gc = uregex_groupCount(tag.regexp, &status);
				if (gc > 0 && !context_stack.empty() && context_stack.back().regexgrps != 0) {
					captureRegex(gc, context_stack.back().regexgrp_ct, context_stack.back().regexgrps, tag);
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
				const Tag& itag = *(grammar->single_tags.find(mter)->second);
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
		for (const auto& mter : reading.tags_numerical) {
			const Tag& itag = *(mter.second);
			if (tag.comparison_hash == itag.comparison_hash) {
				double compval = tag.comparison_val;
				if ((tag.type & T_NUMERIC_MATH) && tag.comparison_offset) {
					MathParser mp(reading.parent->getMin(tag.comparison_hash), reading.parent->getMax(tag.comparison_hash));
					UStringView exp(tag.tag);
					exp.remove_prefix(tag.comparison_offset);
					exp.remove_suffix(1);
					compval = mp.eval(exp);
				}
				else if (compval <= NUMERIC_MIN) {
					compval = reading.parent->getMin(tag.comparison_hash);
				}
				else if (compval >= NUMERIC_MAX) {
					compval = reading.parent->getMax(tag.comparison_hash);
				}

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
	else if (tag.type & (T_VARIABLE|T_LOCAL_VARIABLE)) {
		match = 0;
		auto& vars = [&]() -> auto& {
			if (reading.parent->parent == gWindow->current || !(tag.type & T_LOCAL_VARIABLE)) {
				return variables;
			}
			return reading.parent->parent->variables_set;
		}();

		auto kit = grammar->single_tags.find(tag.comparison_hash);
		if (kit != grammar->single_tags.end()) {
			auto key = kit->second;
			auto it = vars.begin();
			if (key->type & T_REGEXP) {
				it = std::find_if(it, vars.end(), [&](auto& kv) { return doesTagMatchRegexp(kv.first, *key, bypass_index); });
			}
			else if (key->type & T_CASE_INSENSITIVE) {
				it = std::find_if(it, vars.end(), [&](auto& kv) { return doesTagMatchIcase(kv.first, *key, bypass_index); });
			}
			else {
				it = vars.find(tag.comparison_hash);
			}
			if (it != vars.end()) {
				if (tag.variable_hash == 0) {
					match = tag.hash;
				}
				else {
					auto comp = grammar->single_tags.find(tag.variable_hash)->second;
					if (comp->type & T_REGEXP) {
						if (doesTagMatchRegexp(it->second, *comp, bypass_index)) {
							match = tag.hash;
						}
					}
					else if (comp->type & T_CASE_INSENSITIVE) {
						if (doesTagMatchIcase(it->second, *comp, bypass_index)) {
							match = tag.hash;
						}
					}
					else if (comp->hash == it->second) {
						match = tag.hash;
					}
				}
			}
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
		auto sw = reading.parent->parent;
		auto c = std::find(sw->all_cohorts.begin() + reading.parent->local_number, sw->all_cohorts.end(), reading.parent);
		++c;
		if (c != sw->all_cohorts.end() && (*c)->enclosed) {
			match = true;
		}
	}
	else if (tag.type & T_TARGET) {
		if (rule_target && reading.parent == rule_target) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_MARK) {
		if (reading.parent == get_mark()) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_ATTACHTO) {
		if (reading.parent == get_attach_to().cohort) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_SAME_BASIC) {
		if (reading.hash_plain == same_basic) {
			match = grammar->tag_any;
		}
	}
	else if (tag.type & T_CONTEXT) {
		if (context_stack.size() > 1) {
			auto& list = context_stack[context_stack.size()-2].context;
			if (tag.context_ref_pos <= list.size()) {
				if (reading.parent == list[tag.context_ref_pos-1]) {
					match = grammar->tag_any;
				}
			}
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
				if (unif_mode && !check_unif_tags(theset.number, &kv)) {
					continue;
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
 * In the https://edu.visl.dk/cg3_performance.html test data, this function is executed 1073969 times.
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
		auto iiter = theset.trie.lower_bound(grammar->single_tags.find(reading.tags_plain.front())->second);
		auto oiter = reading.tags_plain.lower_bound(theset.trie.begin()->first->hash);
		while (oiter != reading.tags_plain.end() && iiter != theset.trie.end()) {
			if (*oiter == iiter->first->hash) {
				if (iiter->second.terminal) {
					if (unif_mode && !check_unif_tags(theset.number, &*iiter)) {
						++iiter;
						continue;
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
 * In the https://edu.visl.dk/cg3_performance.html test data, this function is executed 5746792 times,
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
		if (index_readingSet_no[set].contains(reading.hash)) {
			return false;
		}
		if (index_readingSet_yes[set].contains(reading.hash)) {
			return true;
		}
	}

	bool retval = false;

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
		// First time, figure out all the sub-sets that match the reading and store them for later comparison
		auto& usets = (*context_stack.back().unif_sets)[theset.number];
		if (usets.empty()) {
			const Set& uset = *grammar->sets_list[theset.sets[0]];
			const size_t size = uset.sets.size();
			for (size_t i = 0; i < size; ++i) {
				const Set& tset = *grammar->sets_list[uset.sets[i]];
				if (doesSetMatchReading(reading, tset.number, bypass_index, ((theset.type & ST_TAG_UNIFY) != 0) | unif_mode)) {
					usets.insert(tset.number);
				}
			}
			retval = !usets.empty();
		}
		// Subsequent times, test whether any of the previously stored sets match the reading
		else {
			auto sets = ss_u32sv.get();
			for (auto usi : usets) {
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
				default:
					throw std::runtime_error("Set operator not implemented!");
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
		if ((unif_mode || (theset.type & ST_TAG_UNIFY)) && !context_stack.empty()) {
			auto unif_tags = context_stack.back().unif_tags;
			const void* tag = nullptr;
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
	const ContextualTest* linked = nullptr;
	Cohort* min = nullptr;
	Cohort* max = nullptr;

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
	auto usets = ss_usets.get();
	uint8_t orz = (context_stack.empty() ? 0 : context_stack.back().regexgrp_ct);

	if (context && !(current_rule->flags & FL_CAPTURE_UNIF) && (theset.type & ST_CHILD_UNIFY) && !context_stack.empty()) {
		*utags = *context_stack.back().unif_tags;
		*usets = *context_stack.back().unif_sets;
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
			if (retval && !context_stack.empty()) {
				context_stack.back().attach_to.cohort = &cohort;
				// This will be set by doesSetMatchCohortNormal
				context_stack.back().attach_to.reading = nullptr;
				context_stack.back().attach_to.subreading = &reading;
			}
		}
	}
	if (!retval && context && !(current_rule->flags & FL_CAPTURE_UNIF) && (theset.type & ST_CHILD_UNIFY) && !context_stack.empty() && (utags->size() != context_stack.back().unif_tags->size() || *utags != *context_stack.back().unif_tags)) {
		context_stack.back().unif_tags->swap(utags);
	}
	if (!retval && context && !(current_rule->flags & FL_CAPTURE_UNIF) && (theset.type & ST_CHILD_UNIFY) && !context_stack.empty() && usets->size() != context_stack.back().unif_sets->size()) {
		context_stack.back().unif_sets->swap(usets);
	}
	if (!retval && !context_stack.empty()) {
		context_stack.back().regexgrp_ct = orz;
	}
	return retval;
}

bool GrammarApplicator::doesSetMatchCohortNormal(Cohort& cohort, const uint32_t set, dSMC_Context* context) {
	bool retval = false;

	if (!(!context || (context->options & (POS_LOOK_DELETED | POS_LOOK_DELAYED | POS_LOOK_IGNORED | POS_NOT))) && (set >= cohort.possible_sets.size() || !cohort.possible_sets.test(set))) {
		return retval;
	}

	const Set* theset = grammar->sets_list[set];

	if (cohort.wread && (!context || !context->in_barrier)) {
		retval = doesSetMatchCohort_helper(cohort, *cohort.wread, *theset, context);
	}

	if (retval && (!context || context->did_test)) {
		return retval;
	}

	ReadingList* lists[4] = { &cohort.readings };
	if (context && (context->options & POS_LOOK_DELETED)) {
		lists[1] = &cohort.deleted;
	}
	if (context && (context->options & POS_LOOK_DELAYED)) {
		lists[2] = &cohort.delayed;
	}
	if (context && (context->options & POS_LOOK_IGNORED)) {
		lists[3] = &cohort.ignored;
	}

	for (auto list : lists) {
		if (list == nullptr) {
			continue;
		}
		for (auto reading : *list) {
			Reading* reading_head = reading;
			if (context && context->test) {
				// ToDo: Barriers need some way to escape sub-readings
				reading = get_sub_reading(reading, context->test->offset_sub);
				if (!reading) {
					continue;
				}
			}
			if (!reading->active && context && (context->options & POS_ACTIVE)) {
				continue;
			}
			if (reading->active && context && (context->options & POS_INACTIVE)) {
				continue;
			}
			if (doesSetMatchCohort_helper(cohort, *reading, *theset, context)) {
				retval = true;
				// if there's a subreading, _helper doesn't know the
				// parent, so set it here
				if (!context_stack.empty() &&
					context_stack.back().attach_to.cohort == &cohort &&
					context_stack.back().attach_to.subreading == reading) {
					context_stack.back().attach_to.reading = reading_head;
				}
			}
			if (retval && (!context || !(context->test && context->test->linked) || context->did_test)) {
				return retval;
			}
		}
	}

	if (context && !context->matched_target && (context->options & POS_NOT)) {
		retval = doesSetMatchCohort_testLinked(cohort, *theset, context);
	}

	if (context && !context->matched_target && !(context->options & (POS_ACTIVE | POS_INACTIVE))) {
		if (!grammar->sets_any || set >= grammar->sets_any->size() || !grammar->sets_any->test(set)) {
			bool was_sub = (context->test && (context->test->offset_sub != 0));
			if (!was_sub && set < cohort.possible_sets.size()) {
				cohort.possible_sets.reset(set);
			}
		}
	}

	return retval;
}

bool GrammarApplicator::doesSetMatchCohortCareful(Cohort& cohort, const uint32_t set, dSMC_Context* context) {
	bool retval = false;

	if (!(!context || (context->options & (POS_LOOK_DELETED | POS_LOOK_DELAYED | POS_LOOK_IGNORED | POS_NOT))) && (set >= cohort.possible_sets.size() || !cohort.possible_sets.test(set))) {
		return retval;
	}

	const Set* theset = grammar->sets_list[set];

	ReadingList* lists[4] = { &cohort.readings };
	if (context && (context->options & POS_LOOK_DELETED)) {
		lists[1] = &cohort.deleted;
	}
	if (context && (context->options & POS_LOOK_DELAYED)) {
		lists[2] = &cohort.delayed;
	}
	if (context && (context->options & POS_LOOK_IGNORED)) {
		lists[3] = &cohort.ignored;
	}

	for (auto list : lists) {
		if (list == nullptr) {
			continue;
		}
		for (auto reading : *list) {
			if (context && context->test) {
				// ToDo: Barriers need some way to escape sub-readings
				reading = get_sub_reading(reading, context->test->offset_sub);
				if (!reading) {
					continue;
				}
			}
			if (!reading->active && context && (context->options & POS_ACTIVE)) {
				continue;
			}
			if (reading->active && context && (context->options & POS_INACTIVE)) {
				continue;
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
