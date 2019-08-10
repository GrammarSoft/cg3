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

#pragma once
#ifndef c6d28b7452ec699b_GRAMMAR_H
#define c6d28b7452ec699b_GRAMMAR_H

#include "stdafx.hpp"
#include "Set.hpp"
#include "Tag.hpp"
#include "Rule.hpp"
#include "sorted_vector.hpp"
#include "interval_vector.hpp"
#include "flat_unordered_set.hpp"

namespace CG3 {
class Anchor;

class Grammar {
public:
	std::ostream* ux_stderr;
	std::ostream* ux_stdout;

	bool has_dep;
	bool has_bag_of_tags;
	bool has_relations;
	bool has_encl_final;
	bool has_protect;
	bool is_binary;
	bool sub_readings_ltr;
	bool ordered = false;
	size_t grammar_size;
	UChar mapping_prefix;
	uint32_t lines;
	uint32_t verbosity_level;
	mutable double total_time;

	std::vector<CG3::Tag*> single_tags_list;
	CG3::Taguint32HashMap single_tags;

	std::vector<CG3::Set*> sets_list;
	CG3::SetSet sets_all;
	CG3::uint32FlatHashMap sets_by_name;
	typedef std::unordered_map<CG3::UString, uint32_t, hash_ustring> set_name_seeds_t;
	set_name_seeds_t set_name_seeds;
	CG3::Setuint32HashMap sets_by_contents;
	CG3::uint32FlatHashMap set_alias;
	CG3::SetSet maybe_used_sets;

	typedef std::vector<CG3::UString> static_sets_t;
	static_sets_t static_sets;

	typedef std::set<URegularExpression*> regex_tags_t;
	regex_tags_t regex_tags;
	typedef CG3::TagSortedVector icase_tags_t;
	icase_tags_t icase_tags;

	typedef std::unordered_map<uint32_t, CG3::ContextualTest*> contexts_t;
	contexts_t templates;
	contexts_t contexts;

	typedef std::unordered_map<uint32_t, CG3::uint32IntervalVector> rules_by_set_t;
	rules_by_set_t rules_by_set;
	typedef std::unordered_map<uint32_t, CG3::uint32IntervalVector> rules_by_tag_t;
	rules_by_tag_t rules_by_tag;
	typedef std::unordered_map<uint32_t, boost::dynamic_bitset<>> sets_by_tag_t;
	sets_by_tag_t sets_by_tag;

	CG3::uint32IntervalVector* rules_any;
	boost::dynamic_bitset<>* sets_any;

	CG3::Set* delimiters;
	CG3::Set* soft_delimiters;
	uint32_t tag_any;
	CG3::uint32Vector preferred_targets;
	CG3::uint32SortedVector reopen_mappings;
	typedef bc::flat_map<uint32_t, uint32_t> parentheses_t;
	parentheses_t parentheses;
	parentheses_t parentheses_reverse;

	CG3::uint32Vector sections;
	CG3::uint32FlatHashMap anchors;

	CG3::RuleVector rule_by_number;
	CG3::RuleVector before_sections;
	CG3::RuleVector rules;
	CG3::RuleVector after_sections;
	CG3::RuleVector null_section;
	CG3::RuleVector wf_rules;

	Grammar();
	~Grammar();

	void addSet(CG3::Set*& to);
	CG3::Set* getSet(uint32_t which) const;
	CG3::Set* allocateSet();
	void destroySet(CG3::Set* set);
	void addSetToList(CG3::Set* s);
	void allocateDummySet();
	uint32_t removeNumericTags(uint32_t s);
	void getTags(const CG3::Set& set, std::set<CG3::TagVector>& rv);

	void addAnchor(const UChar* to, uint32_t at, bool primary = false);
	void addAnchor(const CG3::UString& to, uint32_t at, bool primary = false);

	CG3::Tag* allocateTag();
	CG3::Tag* allocateTag(const UChar* tag);
	CG3::Tag* allocateTag(const CG3::UString& tag);
	CG3::Tag* addTag(CG3::Tag* tag);
	void destroyTag(CG3::Tag* tag);
	void addTagToSet(CG3::Tag* rtag, CG3::Set* set);

	CG3::Rule* allocateRule();
	void addRule(CG3::Rule* rule);
	void destroyRule(CG3::Rule* rule);

	CG3::ContextualTest* allocateContextualTest();
	CG3::ContextualTest* addContextualTest(CG3::ContextualTest* t);
	void addTemplate(CG3::ContextualTest* test, const UChar* name);

	void resetStatistics();
	void reindex(bool unused_sets = false, bool used_tags = false);
	void renameAllRules();

	void indexSetToRule(uint32_t, CG3::Set*);
	void indexTagToRule(uint32_t, uint32_t);
	void indexSets(uint32_t, CG3::Set*);
	void indexTagToSet(uint32_t, uint32_t);
	void setAdjustSets(CG3::Set*);
	void contextAdjustTarget(CG3::ContextualTest*);
};

template<typename Stream>
inline void _trie_unserialize(CG3::trie_t& trie, Stream& input, Grammar& grammar, uint32_t num_tags) {
	for (uint32_t i = 0; i < num_tags; ++i) {
		uint32_t u32tmp = 0;
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = ntoh32(u32tmp);
		trie_node_t& node = trie[grammar.single_tags_list[u32tmp]];

		uint8_t u8tmp = 0;
		fread_throw(&u8tmp, sizeof(uint8_t), 1, input);
		node.terminal = (u8tmp != 0);

		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = ntoh32(u32tmp);
		if (u32tmp) {
			if (!node.trie) {
				node.trie = new CG3::trie_t;
			}
			trie_unserialize(*node.trie, input, grammar, u32tmp);
		}
	}
}

inline void trie_unserialize(CG3::trie_t& trie, FILE* input, Grammar& grammar, uint32_t num_tags) {
	return _trie_unserialize(trie, input, grammar, num_tags);
}

inline void trie_unserialize(CG3::trie_t& trie, std::istream& input, Grammar& grammar, uint32_t num_tags) {
	return _trie_unserialize(trie, input, grammar, num_tags);
}
}

#endif
