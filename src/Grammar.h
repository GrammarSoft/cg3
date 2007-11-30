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
#ifndef __GRAMMAR_H
#define __GRAMMAR_H

#include "stdafx.h"
#include "Set.h"
#include "Tag.h"
#include "CompositeTag.h"
#include "Rule.h"
#include "Anchor.h"

namespace CG3 {

	class Grammar {
	public:
		UFILE *ux_stderr;

		bool has_dep;
		uint32_t last_modified;
		uint32_t grammar_size;
		UChar mapping_prefix;
		UChar *name;
		uint32_t lines, curline;
		stdext::hash_map<uint32_t, Tag*> single_tags;
		stdext::hash_map<uint32_t, CompositeTag*> tags;
		stdext::hash_map<uint32_t, uint32_t> sets_by_name;
		stdext::hash_map<uint32_t, Set*> sets_by_contents;
		stdext::hash_map<uint32_t, uint32_t> set_alias;

		stdext::hash_map<uint32_t, RuleList*> rules_by_tag;

		Set *delimiters;
		Set *soft_delimiters;
		Tag *tag_any;
		std::vector<uint32_t> preferred_targets;

		std::vector<uint32_t> sections;
		std::map<uint32_t, Anchor*> anchors;

		std::map<uint32_t, Rule*> rule_by_line;
		std::vector<Rule*> before_sections;
		std::vector<Rule*> rules;
		std::vector<Rule*> after_sections;

		Grammar();
		~Grammar();

		void setName(const char *to);
		void setName(const UChar *to);

		void trim();

		void addPreferredTarget(UChar *to);

		void addSet(Set *to);
		Set *getSet(uint32_t which);
		Set *allocateSet();
		void destroySet(Set *set);

		void addAnchor(const UChar *to);
		void addAnchor(const UChar *to, const uint32_t line);

		Tag *allocateTag(const UChar *tag);
		void destroyTag(Tag *tag);
		void addTag(Tag *simpletag);
		void addTagToCompositeTag(Tag *simpletag, CompositeTag *tag);

		void addCompositeTag(CompositeTag *tag);
		void addCompositeTagToSet(Set *set, CompositeTag *tag);
		CompositeTag *allocateCompositeTag();
		void destroyCompositeTag(CompositeTag *tag);

		Rule *allocateRule();
		void addRule(Rule *rule, std::vector<Rule*> *where);
		void destroyRule(Rule *rule);

		void reindex();
		void indexRuleToSet(uint32_t, Set*);
		void indexRuleToList(uint32_t, uint32_t);
	};

}

#endif
