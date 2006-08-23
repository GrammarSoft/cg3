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

#include <unicode/ustring.h>
#include "Set.h"
#include "Rule.h"

namespace CG3 {

	class Grammar {
	public:
		unsigned int last_modified;
		UChar *name;
		unsigned int lines;
		unsigned int sections;
		stdext::hash_map<unsigned long, CompositeTag*> tags;
		stdext::hash_map<unsigned long, Set*> sets;
		stdext::hash_map<UChar*, unsigned long> delimiters;
		stdext::hash_map<UChar*, unsigned long> preferred_targets;
		stdext::hash_map<unsigned int, Rule*> rules;

		Grammar() {
			last_modified = 0;
			name = 0;
			lines = 0;
			sections = 0;
		}

		void addDelimiter(UChar *to) {
			UChar *delim = new UChar[u_strlen(to)+1];
			u_strcpy(delim, to);
			delimiters[delim] = hash_sdbm_uchar(delim);
		}
		void addPreferredTarget(UChar *to) {
			UChar *pf = new UChar[u_strlen(to)+1];
			u_strcpy(pf, to);
			preferred_targets[pf] = hash_sdbm_uchar(pf);
		}
		void addSet(Set *to) {
			unsigned long hash = hash_sdbm_uchar(to->name);
			if (sets[hash]) {
				std::wcerr << "Warning: Overwrote set " << to->name << std::endl;
				destroySet(sets[hash]);
			}
			sets[hash] = to;
		}
		Set *getSet(unsigned long which) {
			return sets[which] ? sets[which] : 0;
		}

		Set *allocateSet() {
			return new Set;
		}
		void destroySet(Set *set) {
			delete set;
		}

		void addCompositeTagToSet(Set *set, CompositeTag *tag) {
			if (tag && tag->tags.size()) {
				tag->rehash();
				tags[tag->getHash()] = tag;
				set->addCompositeTag(tag);
			} else {
				std::cerr << "Error: Attempted to add empty tag to grammar and set." << std::endl;
			}
		}
		CompositeTag *allocateCompositeTag() {
			return new CompositeTag;
		}
		void destroyCompositeTag(CompositeTag *tag) {
			delete tag;
		}

		void manipulateSet(unsigned long set_a, int op, unsigned long set_b, unsigned long result) {
			if (op <= S_IGNORE || op >= STRINGS_COUNT) {
				std::wcerr << "Error: Invalid set operation on line " << lines << std::endl;
				return;
			}
			if (!sets[set_a]) {
				std::wcerr << "Error: Invalid left operand for set operation on line " << lines << std::endl;
				return;
			}
			if (!sets[set_b]) {
				std::wcerr << "Error: Invalid right operand for set operation on line " << lines << std::endl;
				return;
			}
			if (!result) {
				std::wcerr << "Error: Invalid target for set operation on line " << lines << std::endl;
				return;
			}
			stdext::hash_map<unsigned long, CompositeTag*> result_tags;
			switch (op) {
				case S_OR:
				{
					stdext::hash_map<unsigned long, CompositeTag*>::iterator iter;
					for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
						result_tags[iter->first] = iter->second;
					}
					for (iter = sets[set_b]->tags.begin() ; iter != sets[set_b]->tags.end() ; iter++) {
						result_tags[iter->first] = iter->second;
					}
					break;
				}
				case S_DENY:
				case S_NOT:
				case S_MINUS:
				{
					stdext::hash_map<unsigned long, CompositeTag*>::iterator iter;
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
					stdext::hash_map<unsigned long, CompositeTag*>::iterator iter_a;
					for (iter_a = sets[set_a]->tags.begin() ; iter_a != sets[set_a]->tags.end() ; iter_a++) {
						stdext::hash_map<unsigned long, CompositeTag*>::iterator iter_b;
						for (iter_b = sets[set_b]->tags.begin() ; iter_b != sets[set_b]->tags.end() ; iter_b++) {
							CompositeTag *tag_r = allocateCompositeTag();

							stdext::hash_map<unsigned long, Tag*>::iterator iter_t;
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
					std::wcerr << "Error: Invalid set operation " << op << " between " << sets[set_a]->getName() << " and " << sets[set_b]->getName() << std::endl;
					break;
				}
			}
			sets[result]->tags.clear();
			sets[result]->tags.swap(result_tags);
		}
	};

}

#endif
