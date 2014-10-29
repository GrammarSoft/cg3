/*
* Copyright (C) 2007-2014, GrammarSoft ApS
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
		UFILE *ux_stderr, *ux_stdout;

		bool has_dep;
		bool has_relations;
		bool has_encl_final;
		bool is_binary;
		bool sub_readings_ltr;
		size_t grammar_size;
		UChar mapping_prefix;
		uint32_t lines;
		uint32_t verbosity_level;
		mutable double total_time;

		std::vector<Tag*> single_tags_list;
		Taguint32HashMap single_tags;

		std::vector<Set*> sets_list;
		SetSet sets_all;
		uint32FlatHashMap sets_by_name;
		typedef stdext::hash_map<UString,uint32_t> set_name_seeds_t;
		set_name_seeds_t set_name_seeds;
		Setuint32HashMap sets_by_contents;
		uint32FlatHashMap set_alias;

		typedef std::vector<UString> static_sets_t;
		static_sets_t static_sets;

		typedef std::set<URegularExpression*> regex_tags_t;
		regex_tags_t regex_tags;
		typedef TagSortedVector icase_tags_t;
		icase_tags_t icase_tags;

		std::vector<ContextualTest*> template_list;
		typedef stdext::hash_map<uint32_t, ContextualTest*> contexts_t;
		contexts_t templates;
		contexts_t contexts;
		ContextVector contexts_list;

		typedef stdext::hash_map<uint32_t, uint32IntervalVector> rules_by_set_t;
		rules_by_set_t rules_by_set;
		typedef stdext::hash_map<uint32_t, uint32IntervalVector> rules_by_tag_t;
		rules_by_tag_t rules_by_tag;
		typedef stdext::hash_map<uint32_t, uint32FlatHashSet> sets_by_tag_t;
		sets_by_tag_t sets_by_tag;

		uint32IntervalVector *rules_any;
		uint32FlatHashSet *sets_any;

		Set *delimiters;
		Set *soft_delimiters;
		uint32_t tag_any;
		uint32Vector preferred_targets;
		typedef bc::flat_map<uint32_t,uint32_t> parentheses_t;
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

		Grammar();
		~Grammar();

		void addPreferredTarget(UChar *to);

		void addSet(Set *& to);
		Set *getSet(uint32_t which) const;
		Set *allocateSet();
		void destroySet(Set *set);
		void addSetToList(Set *s);
		Set *parseSet(const UChar *name);

		void addAnchor(const UChar *to, uint32_t at, bool primary = false);

		Tag *allocateTag();
		Tag *allocateTag(const UChar *tag, bool raw = false);
		void destroyTag(Tag *tag);
		void addTagToSet(Tag *rtag, Set *set);

		Rule *allocateRule();
		void addRule(Rule *rule);
		void destroyRule(Rule *rule);

		ContextualTest *allocateContextualTest();
		ContextualTest *addContextualTest(ContextualTest *t);
		void addTemplate(ContextualTest *test, const UChar *name);

		void resetStatistics();
		void reindex(bool unused_sets=false);
		void renameAllRules();

		void indexSetToRule(uint32_t, Set*);
		void indexTagToRule(uint32_t, uint32_t);
		void indexSets(uint32_t, Set*);
		void indexTagToSet(uint32_t, uint32_t);
	};

}

#endif
