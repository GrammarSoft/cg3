/*
* Copyright (C) 2007-2015, GrammarSoft ApS
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

#include "Relabeller.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"
#include "TagTrie.hpp"

namespace CG3 {

Relabeller::Relabeller(Grammar& res, const Grammar& relabels, UFILE *ux_err) :
	ux_stderr(ux_err),
	grammar(&res),
	relabels(&relabels)
{
	UStringMap* as_tag = new UStringMap;
	UStringSetMap* as_list = new UStringSetMap;
	UStringSetMap* as_set = new UStringSetMap;

	std::cerr << ", Sets: " << relabels.sets_list.size() << ", Tags: " << relabels.single_tags.size() << std::endl;
	if (relabels.rules_any) {
		std::cerr << relabels.rules_any->size() << " relabel rules cannot be skipped by index(?)" << std::endl;
	}

	for(const auto & rule : relabels.rule_by_number) {
		const TagVector& fromTags = trie_getTagList(rule->maplist->trie);
		Set* target = relabels.sets_list[rule->target];
		const TagVector& toTags = trie_getTagList(target->trie);
		u_fprintf(ux_stderr, "rule->target:%d, line:%d, name:%S, maplist.size:%d, target.size: %d\n", rule->target, rule->line, rule->name, fromTags.size(), toTags.size());
		if(!(rule->maplist->trie_special.empty() && target->trie_special.empty())) {
			u_fprintf(ux_stderr, "Warning: Relabel rule '%S' on line %d has %d special tags, skipping!\n", rule->name, rule->line);
			continue;
		}
		if(fromTags.size()!=1) {
			u_fprintf(ux_stderr, "Warning: Relabel rule '%S' on line %d has %d tags in the maplist (expecting 1), skipping!\n", rule->name, rule->line, fromTags.size());
			continue;
		}
		Tag *fromTag = fromTags[0];
		if(toTags.size() == 1) {
			Tag *toTag = toTags[0];
			as_tag->emplace(fromTag->tag.c_str(), toTag->tag.c_str());
			u_fprintf(ux_stderr, "\t%S -> %S\n", fromTag->tag.c_str(), toTag->tag.c_str());
		}
		else if(toTags.size() > 1) {
			as_list->emplace(fromTag->tag.c_str(), target);
			u_fprintf(ux_stderr, "\t%S -> (LIST)\n", fromTag->tag.c_str());
			boost_foreach (const TagVector::value_type toit, toTags) {
				if(toit->type & T_SPECIAL) {
					u_fprintf(ux_stderr, "Warning: Special tags (%S) not supported yet.\n", toit->tag.c_str());
				}
			}
		}
		else {		// if(toTags.size()==0)
			as_set->emplace(fromTag->tag.c_str(), target);
			u_fprintf(ux_stderr, "\t%S -> (SET)\n", fromTag->tag.c_str());
		}
	}

	relabel_as_tag = as_tag;
	relabel_as_list = as_list;
	relabel_as_set = as_set;
}

Relabeller::~Relabeller() {
        delete relabel_as_tag;
        relabel_as_tag = 0;
        delete relabel_as_list;
        relabel_as_list = 0;
	delete relabel_as_set;
        relabel_as_set = 0;
}

uint32_t Relabeller::addRelabelSet(Set* s_r) { // TODO: probably deprecated
	Set* s_g = grammar->allocateSet();

	uint32_t nsets = s_r->sets.size();
	u_fprintf(ux_stderr, "\t\ts_r %d: addRelabelSet(s_r=%p, number=%d), child sets: %d\n", s_r->number, s_r, s_r->number, nsets);
	s_g->sets.resize(nsets);
	for (uint32_t i=0 ; i<nsets; ++i) {
		// First ensure all referred-to sets exist:
		uint32_t child_num_r = s_r->sets[i];
		uint32_t child_num_g = addRelabelSet(relabels->sets_list[child_num_r]);
		u_fprintf(ux_stderr, "\t\ts_r %d: child %d = set num %d (s_r child num %d)\n", s_r->number, i, child_num_g, child_num_r);
		s_g->sets[i] = child_num_g;
	}
	// for (uint32_t i=0 ; i<s_g->sets.size(); ++i) {
	// 	u_fprintf(ux_stderr, "s_r %d: child %d = set num %d\n", s_r->number, i, s_g->sets[i]);
	// }
	// foreach (uint32Vector, s_g->sets, sit, sit_end) {
	// 	Set* list_set = grammar->sets_list[*sit];
	// 	u_fprintf(ux_stderr, "s_r %d: sit %p num %d, getSet(*sit) %p\n", s_r->number, sit, list_set->number);
	// }

	uint32_t nset_ops = s_r->set_ops.size();
	s_g->set_ops.resize(nset_ops);
	for (uint32_t i=0 ; i < nset_ops; ++i) {
		s_g->set_ops[i] = s_r->set_ops[i]; // enum from Strings.cpp, same across grammars
	}

	s_g->trie = trie_copy(s_r->trie, *grammar);
	s_g->trie_special = trie_copy(s_r->trie_special, *grammar);
	s_g->ff_tags = s_r->ff_tags;
	s_g->type = s_r->type;
	u_fprintf(ux_stderr, "\t\t\ts_r %d addSet(s_g=%p, number=%d)\n", s_r->number, s_g, s_g->number);
	s_g->setName(grammar->sets_list.size()+100);
	grammar->sets_list.push_back(s_g);
	s_g->number = (uint32_t)grammar->sets_list.size()-1;
	u_fprintf(ux_stderr, "\t\t\ts_r %d copied to s_g %d\n", s_r->number, s_g->number);
	return s_g->number;
}

// From TextualParser::parseTagList
struct freq_sorter {
	const bc::flat_map<Tag*, size_t>& tag_freq;

	freq_sorter(const bc::flat_map<Tag*, size_t>& tag_freq) : tag_freq(tag_freq) {
	}

	bool operator()(Tag *a, Tag *b) const {
		// Sort highest frequency first
		return tag_freq.find(a)->second > tag_freq.find(b)->second;
	}
};

TagVector Relabeller::transferTags(const TagVector tv_r) {
	TagVector tv_g;
	boost_foreach (Tag* tag_r, tv_r) {
		Tag* tag_g = new Tag(*tag_r);
		tag_g = grammar->addTag(tag_g); // new is deleted if it exists
		tv_g.push_back(tag_g);
	}
	return tv_g;
}

void Relabeller::relabel() {
	stdext::hash_map<UString, Tag* > tag_by_str;
	// RELABEL AS TAG:
	for(const auto & fromTag : grammar->single_tags_list) {
		UString tagName = fromTag->toUString(true);
		BOOST_AUTO(const toTag, relabel_as_tag->find(tagName));
		if(toTag != relabel_as_tag->end()) {
		 	fromTag->tag.assign(toTag->second);
		 	fromTag->rehash();
		}
		tag_by_str[fromTag->tag] = fromTag;
	}
	stdext::hash_map<UString, std::set<Set* > > sets_by_tag;
	boost_foreach (const std::vector<Set*>::value_type it, grammar->sets_list) {
		const TagVector& toTags = trie_getTagList(it->trie);
		boost_foreach (const TagVector::value_type toit, toTags) {
			sets_by_tag[toit->tag].insert(it);
		}
	}
	// RELABEL AS LIST:
	boost_foreach (const UStringSetMap::value_type& it, *relabel_as_list) {
		Set* set_r = relabels->sets_list[it.second->number];
		std::set<TagVector> to_tvs = trie_getTagsOrdered(set_r->trie);
		u_fprintf(ux_stderr, "Relabelling %S to a LIST ", it.first.c_str()); printTrie(set_r->trie, ux_stderr); u_fprintf(ux_stderr, "\n");

		BOOST_AUTO(const sets_g, sets_by_tag.find(it.first));
		if(sets_g != sets_by_tag.end()) {
			boost_foreach(Set* set_g, sets_g->second) {
				u_fprintf(ux_stderr, "Splicing into SET s = "); printTrie(set_g->trie, ux_stderr); u_fprintf(ux_stderr, "\n");
				std::set<TagVector> old_tvs = trie_getTagsOrdered(set_g->trie);
				trie_delete(set_g->trie);
				set_g->trie.clear();

				std::set<TagVector> taglists;
				bc::flat_map<Tag*, size_t> tag_freq;
				boost_foreach (const TagVector& old_tags, old_tvs) {
					TagVector tags_except_from;

					bool seen = false;
					boost_foreach (Tag* old_tag, old_tags) {
						if(old_tag->hash == tag_by_str[it.first]->hash) {
							seen = true;
						}
						else {
							tags_except_from.push_back(old_tag);
						}
					}
					std::set<TagVector> suffixes;
					if(seen) {
						suffixes = trie_getTagsOrdered(set_r->trie);
					}
					else {
						TagVector dummy;
						suffixes.insert(dummy);
					}
					boost_foreach(const TagVector& tv, suffixes) {
						TagVector tags = TagVector(tags_except_from);
						tags.insert(tags.end(), tv.begin(), tv.end());
						tags = transferTags(tags);
						// From TextualParser::parseTagList
						std::sort(tags.begin(), tags.end());
						tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
						if (taglists.insert(tags).second) {
							boost_foreach (Tag *t, tags) {
								++tag_freq[t];
							}
						}
					}
				}
				// From TextualParser::parseTagList
				freq_sorter fs(tag_freq);
				boost_foreach (const TagVector& tvc, taglists) {
					if (tvc.size() == 1) {
						grammar->addTagToSet(tvc[0], set_g);
						continue;
					}
					TagVector& tv = const_cast<TagVector&>(tvc);
					std::sort(tv.begin(), tv.end(), fs);
					trie_insert(set_g->trie, tv);
				}
				u_fprintf(ux_stderr, "After splicing, SET s = "); printTrie(set_g->trie, ux_stderr); u_fprintf(ux_stderr, "\n");
			}
		}
	}
	// RELABEL AS SET:
	boost_foreach (const UStringSetMap::value_type& it, *relabel_as_set) {
		u_fprintf(ux_stderr, "Relabelling %S to a SET (TODO)\n", it.first.c_str());
		// Set* set_r = relabels->sets_list[it.second->number];
		// uint32_t s_num_g = addRelabelSet(set_r);
		// Set* set_g = grammar->sets_list[s_num_g];
		// u_fprintf(ux_stderr, "\tset_r %d, sh %d\t=>", set_r->number, set_r->hash);
		// u_fprintf(ux_stderr, "\ttag %S to set_g %d==%d, sh %d\n", it.first.c_str(), set_g->number, s_num_g, set_g->hash);

		// TODO: now we have to find all the places where fromTag is used, and change all those sets s to be what they were, minus the fromTag, plus set_g
		// If fromTag=N    and the set s is a plain list N adv, then we get sets=s_g OR adv
		// If fromTag=Prop and the set s is a list (N Prop Gen) adv, then … is that represented as list or set?

		// TODO: go through each set in grammar, if it has only a single tag, we simply replace that set with the new one, keeping track of which ones we replace in a map
		// if it has no tags, it has only references to other sets, so we (after replacing actual sets) replace those references using our map
		// if it has more than one tag, we need to turn it into a no-tag set referring to an OR-ed list of new single-tag sets (except we need to + the parenthesized ones, but how do we find them?)
	}
	grammar->reindex();
}

}
