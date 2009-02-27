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

#ifndef __GRAMMAR_H
#define __GRAMMAR_H

#include "stdafx.h"
#include "Set.h"
#include "Tag.h"
#include "CompositeTag.h"
#include "Rule.h"
#include "Anchor.h"
#include "ContextualTest.h"

namespace CG3 {

	class Grammar {
	public:
		UFILE *ux_stderr;

		bool has_dep;
		bool has_encl_final;
		bool is_binary;
		uint32_t grammar_size;
		UChar mapping_prefix;
		uint32_t lines;
		mutable double total_time;

		std::vector<Tag*> single_tags_list;
		stdext::hash_map<uint32_t, Tag*> single_tags;

		std::vector<CompositeTag*> tags_list;
		stdext::hash_map<uint32_t, CompositeTag*> tags;

		std::vector<Set*> sets_list;
		std::set<Set*> sets_all;
		uint32HashMap sets_by_name;
		uint32HashMap set_name_seeds;
		Setuint32HashMap sets_by_contents;
		uint32HashMap set_alias;

		std::vector<ContextualTest*> template_list;
		stdext::hash_map<uint32_t, ContextualTest*> templates;

		stdext::hash_map<uint32_t, uint32Set*> rules_by_set;
		uint32HashSetuint32HashMap rules_by_tag;
		uint32HashSetuint32HashMap sets_by_tag;

		uint32HashSet *rules_any;
		uint32HashSet *sets_any;

		Set *delimiters;
		Set *soft_delimiters;
		uint32_t tag_any;
		uint32Vector preferred_targets;
		uint32Map parentheses;
		uint32Map parentheses_reverse;

		uint32Vector sections;
		uint32Map anchor_by_hash;
		std::map<uint32_t, Anchor*> anchor_by_line;

		RuleByLineHashMap rule_by_line;
		RuleVector before_sections;
		RuleVector rules;
		RuleVector after_sections;
		RuleVector null_section;

		Grammar();
		~Grammar();

		void addPreferredTarget(UChar *to);

		void addSet(Set *to);
		Set *getSet(uint32_t which);
		Set *allocateSet();
		void destroySet(Set *set);
		void addSetToList(Set *s);

		void addAnchor(const UChar *to, const uint32_t line);

		Tag *allocateTag();
		Tag *allocateTag(const UChar *tag);
		void destroyTag(Tag *tag);
		void addTagToCompositeTag(Tag *simpletag, CompositeTag *tag);

		CompositeTag *addCompositeTag(CompositeTag *tag);
		CompositeTag *addCompositeTagToSet(Set *set, CompositeTag *tag);
		CompositeTag *allocateCompositeTag();
		void destroyCompositeTag(CompositeTag *tag);

		Rule *allocateRule();
		void addRule(Rule *rule);
		void destroyRule(Rule *rule);

		ContextualTest *allocateContextualTest();
		void addContextualTest(ContextualTest *test, const UChar *name);

		void resetStatistics();
		void reindex();
		void renameAllRules();

		void indexSetToRule(uint32_t, Set*);
		void indexTagToRule(uint32_t, uint32_t);
		void indexSets(uint32_t, Set*);
		void indexTagToSet(uint32_t, uint32_t);
	};

}

#endif
