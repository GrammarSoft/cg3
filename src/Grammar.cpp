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
#include "stdafx.h"
#include <unicode/ustring.h>
#include "Grammar.h"
#include "Set.h"
#include "Section.h"
#include "Rule.h"

namespace CG3 {

	Grammar::Grammar() {
		last_modified = 0;
		name = 0;
		lines = 0;
		curline = 0;
		delimiters = 0;
		srand((uint32_t)time(0));
	}
	
	Grammar::~Grammar() {
		if (name) {
			delete name;
		}
		
		std::vector<UChar*>::iterator iter;
		for (iter = preferred_targets.begin() ; iter != preferred_targets.end() ; iter++) {
			if (*iter) {
				delete *iter;
			}
		}
		preferred_targets.clear();
		
		std::map<uint32_t, Section*>::iterator iter_sec;
		for (iter_sec = sections.begin() ; iter_sec != sections.end() ; iter_sec++) {
			if (iter_sec->second) {
				delete iter_sec->second;
			}
		}
		sections.clear();
		
		stdext::hash_map<uint32_t, Set*>::iterator iter_set;
		for (iter_set = sets.begin() ; iter_set != sets.end() ; iter_set++) {
			if (iter_set->second) {
				delete iter_set->second;
			}
		}
		sets.clear();
		
		stdext::hash_map<uint32_t, CompositeTag*>::iterator iter_ctag;
		for (iter_ctag = tags.begin() ; iter_ctag != tags.end() ; iter_ctag++) {
			if (iter_ctag->second) {
				delete iter_ctag->second;
			}
		}
		tags.clear();
	}

	void Grammar::addPreferredTarget(UChar *to) {
		UChar *pf = new UChar[u_strlen(to)+1];
		u_strcpy(pf, to);
		preferred_targets.push_back(pf);
	}
	void Grammar::addSet(Set *to) {
		uint32_t hash = hash_sdbm_uchar(to->name);
		if (sets[hash]) {
			u_fprintf(ux_stderr, "Warning: Overwrote set %S.\n", to->name);
			destroySet(sets[hash]);
		}
		sets[hash] = to;
	}
	Set *Grammar::getSet(uint32_t which) {
		return sets[which] ? sets[which] : 0;
	}

	Set *Grammar::allocateSet() {
		return new Set;
	}
	void Grammar::destroySet(Set *set) {
		delete set;
	}

	void Grammar::addCompositeTagToSet(Set *set, CompositeTag *tag) {
		if (tag && tag->tags.size()) {
			tag->rehash();
			tags[tag->getHash()] = tag;
			set->addCompositeTag(tag);
		} else {
			u_fprintf(ux_stderr, "Error: Attempted to add empty tag to grammar and set!\n");
		}
	}
	CompositeTag *Grammar::allocateCompositeTag() {
		return new CompositeTag;
	}
	CompositeTag *Grammar::duplicateCompositeTag(CompositeTag *tag) {
		CompositeTag *tmp = new CompositeTag;
		std::map<uint32_t, Tag*>::iterator iter;
		for (iter = tag->tags_map.begin() ; iter != tag->tags_map.end() ; iter++) {
			Tag *ntag = tmp->duplicateTag(iter->second);
			tmp->addTag(ntag);
		}
		return tmp;
	}
	void Grammar::destroyCompositeTag(CompositeTag *tag) {
		delete tag;
	}

	Rule *Grammar::allocateRule() {
		return new Rule;
	}
	void Grammar::destroyRule(Rule *rule) {
		delete rule;
	}

	// ToDO: Implement the rest
	void Grammar::manipulateSet(uint32_t set_a, int op, uint32_t set_b, uint32_t result) {
		if (op <= S_IGNORE || op >= STRINGS_COUNT) {
			u_fprintf(ux_stderr, "Error: Invalid set operation on line %u!\n", lines);
			return;
		}
		if (!sets[set_a]) {
			u_fprintf(ux_stderr, "Error: Invalid left operand for set operation on line %u!\n", lines);
			return;
		}
		if (!sets[set_b]) {
			u_fprintf(ux_stderr, "Error: Invalid right operand for set operation on line %u!\n", lines);
			return;
		}
		if (!result) {
			u_fprintf(ux_stderr, "Error: Invalid target for set operation on line %u!\n", lines);
			return;
		}
		stdext::hash_map<uint32_t, CompositeTag*> result_tags;
		switch (op) {
			case S_OR:
			{
				stdext::hash_map<uint32_t, CompositeTag*>::iterator iter;
				for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
					result_tags[iter->first] = iter->second;
				}
				for (iter = sets[set_b]->tags.begin() ; iter != sets[set_b]->tags.end() ; iter++) {
					result_tags[iter->first] = iter->second;
				}
				break;
			}
			case S_DENY:
			{
				stdext::hash_map<uint32_t, CompositeTag*>::iterator iter;
				for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
					if (!sets[set_b]->tags[iter->first]) {
						result_tags[iter->first] = iter->second;
					}
				}
				for (iter = sets[set_b]->tags.begin() ; iter != sets[set_b]->tags.end() ; iter++) {
					CompositeTag *tmp = duplicateCompositeTag(iter->second);
					std::map<uint32_t, Tag*>::iterator iter_tag;
					for (iter_tag = tmp->tags_map.begin() ; iter_tag != tmp->tags_map.end() ; iter_tag++) {
						iter_tag->second->denied = !(iter_tag->second->denied);
					}
					result_tags[iter->first] = tmp;
				}
				break;
			}
			case S_NOT:
			{
				stdext::hash_map<uint32_t, CompositeTag*>::iterator iter;
				for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
					if (!sets[set_b]->tags[iter->first]) {
						result_tags[iter->first] = iter->second;
					}
				}
				for (iter = sets[set_b]->tags.begin() ; iter != sets[set_b]->tags.end() ; iter++) {
					CompositeTag *tmp = duplicateCompositeTag(iter->second);
					std::map<uint32_t, Tag*>::iterator iter_tag;
					for (iter_tag = tmp->tags_map.begin() ; iter_tag != tmp->tags_map.end() ; iter_tag++) {
						iter_tag->second->negative = !(iter_tag->second->negative);
					}
					result_tags[iter->first] = tmp;
				}
				break;
			}
			case S_MINUS:
			{
				stdext::hash_map<uint32_t, CompositeTag*>::iterator iter;
				for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
					if (!sets[set_b]->tags[iter->first]) {
						result_tags[iter->first] = iter->second;
					}
				}
				break;
			}
			case S_MULTIPLY:
			case S_PLUS:
			{
				stdext::hash_map<uint32_t, CompositeTag*>::iterator iter_a;
				for (iter_a = sets[set_a]->tags.begin() ; iter_a != sets[set_a]->tags.end() ; iter_a++) {
					stdext::hash_map<uint32_t, CompositeTag*>::iterator iter_b;
					for (iter_b = sets[set_b]->tags.begin() ; iter_b != sets[set_b]->tags.end() ; iter_b++) {
						CompositeTag *tag_r = allocateCompositeTag();

						stdext::hash_map<uint32_t, Tag*>::iterator iter_t;
						for (iter_t = iter_a->second->tags.begin() ; iter_t != iter_a->second->tags.end() ; iter_t++) {
							Tag *ttag = tag_r->allocateTag(iter_t->second->raw);
							tag_r->addTag(ttag);
						}

						for (iter_t = iter_b->second->tags.begin() ; iter_t != iter_b->second->tags.end() ; iter_t++) {
							Tag *ttag = tag_r->allocateTag(iter_t->second->raw);
							tag_r->addTag(ttag);
						}
						result_tags[tag_r->rehash()] = tag_r;
					}
				}
				break;
			}
			default:
			{
				u_fprintf(ux_stderr, "Error: Invalid set operation %u between %S and %S!\n", op, sets[set_a]->getName(), sets[set_b]->getName());
				break;
			}
		}
		sets[result]->tags.clear();
		sets[result]->tags.swap(result_tags);
	}

}
