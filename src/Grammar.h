/*
* Copyright (C) 2007, GrammarSoft Aps
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

namespace CG3 {

	class Grammar {
	public:
		UFILE *ux_stderr;

		bool has_dep;
		bool is_binary;
		time_t last_modified;
		uint32_t grammar_size;
		UChar mapping_prefix;
		UChar *name;
		uint32_t lines, curline;
		mutable clock_t total_time;
		stdext::hash_map<uint32_t, Tag*> single_tags;
		stdext::hash_map<uint32_t, CompositeTag*> tags;
		std::set<Set*> sets_all;
		stdext::hash_map<uint32_t, uint32_t> sets_by_name;
		stdext::hash_map<uint32_t, Set*> sets_by_contents;
		stdext::hash_map<uint32_t, uint32_t> set_alias;

		stdext::hash_map<uint32_t, uint32HashSet*> rules_by_tag;
		stdext::hash_map<uint32_t, uint32HashSet*> sets_by_tag;

		Set *delimiters;
		Set *soft_delimiters;
		uint32_t tag_any;
		uint32Vector preferred_targets;

		uint32Vector sections;
		std::map<uint32_t, Anchor*> anchors;

		std::map<uint32_t, Rule*> rule_by_line;
		std::vector<Rule*> before_sections;
		std::vector<Rule*> rules;
		std::vector<Rule*> after_sections;

		Grammar();
		~Grammar();

		void setName(const char *to);
		void setName(const UChar *to);

		void addPreferredTarget(UChar *to);

		void addSet(Set *to);
		Set *getSet(uint32_t which);
		Set *allocateSet();
		void destroySet(Set *set);

		void addAnchor(const UChar *to);
		void addAnchor(const UChar *to, const uint32_t line);

		Tag *allocateTag();
		Tag *allocateTag(const UChar *tag);
		void destroyTag(Tag *tag);
		Tag *addTag(Tag *simpletag);
		void addTagToCompositeTag(Tag *simpletag, CompositeTag *tag);

		void addCompositeTag(CompositeTag *tag);
		void addCompositeTagToSet(Set *set, CompositeTag *tag);
		CompositeTag *allocateCompositeTag();
		void destroyCompositeTag(CompositeTag *tag);

		Rule *allocateRule();
		void addRule(Rule *rule);
		void destroyRule(Rule *rule);

		void resetStatistics();
		void reindex();

		void indexSetToRule(uint32_t, Set*);
		void indexTagToRule(uint32_t, uint32_t);
		void indexSets(uint32_t, Set*);
		void indexTagToSet(uint32_t, uint32_t);
	};

}

#endif
