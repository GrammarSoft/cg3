/*
* Copyright (C) 2007-2023, GrammarSoft ApS
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
	std::ostream* ux_stderr = nullptr;
	std::ostream* ux_stdout = nullptr;

	bool has_dep = false;
	bool has_bag_of_tags = false;
	bool has_relations = false;
	bool has_encl_final = false;
	bool has_protect = false;
	bool is_binary = false;
	bool sub_readings_ltr = false;
	bool ordered = false;
	bool addcohort_attach = false;
	size_t grammar_size = 0;
	size_t num_tags = 0;
	UChar mapping_prefix = '@';
	uint32_t lines = 0;
	uint32_t verbosity_level = 0;
	mutable double total_time = 0;

	std::vector<Tag*> single_tags_list;
	Taguint32HashMap single_tags;

	std::vector<Set*> sets_list;
	SetSet sets_all;
	uint32FlatHashMap sets_by_name;
	typedef std::unordered_map<UString, uint32_t, hash_ustring> set_name_seeds_t;
	set_name_seeds_t set_name_seeds;
	Setuint32HashMap sets_by_contents;
	uint32FlatHashMap set_alias;
	SetSet maybe_used_sets;

	typedef std::vector<UString> static_sets_t;
	static_sets_t static_sets;

	typedef std::set<URegularExpression*> regex_tags_t;
	regex_tags_t regex_tags;
	typedef TagSortedVector icase_tags_t;
	icase_tags_t icase_tags;

	typedef std::unordered_map<uint32_t, ContextualTest*> contexts_t;
	contexts_t templates;
	contexts_t contexts;

	typedef std::unordered_map<uint32_t, uint32IntervalVector> rules_by_set_t;
	rules_by_set_t rules_by_set;
	typedef std::unordered_map<uint32_t, uint32IntervalVector> rules_by_tag_t;
	rules_by_tag_t rules_by_tag;
	typedef std::unordered_map<uint32_t, boost::dynamic_bitset<>> sets_by_tag_t;
	sets_by_tag_t sets_by_tag;

	uint32IntervalVector* rules_any = nullptr;
	boost::dynamic_bitset<>* sets_any = nullptr;

	Set* delimiters = nullptr;
	Set* soft_delimiters = nullptr;
	Set* text_delimiters = nullptr;
	uint32_t tag_any = 0;
	uint32Vector preferred_targets;
	uint32SortedVector reopen_mappings;
	typedef bc::flat_map<uint32_t, uint32_t> parentheses_t;
	parentheses_t parentheses;
	parentheses_t parentheses_reverse;

	uint32Vector sections;
	uint32FlatHashMap anchors;

	RuleVector rule_by_number;
	RuleVector before_sections;
	RuleVector rules;
	RuleVector after_sections;
	RuleVector null_section;
	RuleVector wf_rules;

	Grammar() = default;
	~Grammar();

	void addSet(Set*& to);
	Set* getSet(uint32_t which) const;
	void appendToSet(Set*& to);
	Set* undefSet(const UString& name);
	Set* allocateSet();
	void destroySet(Set* set);
	void addSetToList(Set* s);
	void allocateDummySet();
	uint32_t removeNumericTags(uint32_t s);
	void getTags(const Set& set, TagVectorSet& rv) const;
	TagList getTagList_Any(const Set& theSet) const;
	void getTagList_Any(const Set& theSet, TagList& theTags) const;

	void addAnchor(const UChar* to, uint32_t at, bool primary = false);
	void addAnchor(const UString& to, uint32_t at, bool primary = false);
	void addAnchor(const UStringView& to, uint32_t at, bool primary = false) {
		return addAnchor(to.data(), at, primary);
	}

	Tag* allocateTag();
	Tag* allocateTag(const UChar* tag);
	Tag* allocateTag(const UString& tag);
	Tag* allocateTag(const UStringView& tag) {
		return allocateTag(tag.data());
	}
	Tag* addTag(Tag* tag);
	void destroyTag(Tag* tag);
	void addTagToSet(Tag* rtag, Set* set);

	Rule* allocateRule();
	void addRule(Rule* rule);
	void destroyRule(Rule* rule);

	ContextualTest* allocateContextualTest();
	ContextualTest* addContextualTest(ContextualTest* t);
	void addTemplate(ContextualTest* test, const UChar* name);

	void reindex(bool unused_sets = false, bool used_tags = false);

	void indexSetToRule(uint32_t, Set*);
	void indexTagToRule(uint32_t, uint32_t);
	void indexSets(uint32_t, Set*);
	void indexTagToSet(uint32_t, uint32_t);
	void setAdjustSets(Set*);
	void contextAdjustTarget(ContextualTest*);
};

template<typename Stream>
inline void _trie_unserialize(trie_t& trie, Stream& input, Grammar& grammar, uint32_t num_tags) {
	for (uint32_t i = 0; i < num_tags; ++i) {
		uint32_t u32tmp = 0;
		fread_throw(&u32tmp, sizeof(u32tmp), 1, input);
		u32tmp = ntoh32(u32tmp);
		trie_node_t& node = trie[grammar.single_tags_list[u32tmp]];

		uint8_t u8tmp = 0;
		fread_throw(&u8tmp, sizeof(u8tmp), 1, input);
		node.terminal = (u8tmp != 0);

		fread_throw(&u32tmp, sizeof(u32tmp), 1, input);
		u32tmp = ntoh32(u32tmp);
		if (u32tmp) {
			if (!node.trie) {
				node.trie.reset(new trie_t);
			}
			trie_unserialize(*node.trie, input, grammar, u32tmp);
		}
	}
}

inline void trie_unserialize(trie_t& trie, FILE* input, Grammar& grammar, uint32_t num_tags) {
	return _trie_unserialize(trie, input, grammar, num_tags);
}

inline void trie_unserialize(trie_t& trie, std::istream& input, Grammar& grammar, uint32_t num_tags) {
	return _trie_unserialize(trie, input, grammar, num_tags);
}
}

#endif
