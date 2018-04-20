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

#include "Relabeller.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"
#include "TagTrie.hpp"

namespace CG3 {

Relabeller::Relabeller(Grammar& res, const Grammar& relabels, std::ostream& ux_err)
  : ux_stderr(&ux_err)
  , grammar(&res)
  , relabels(&relabels)
{
	std::unique_ptr<UStringSetMap> as_list{ new UStringSetMap };
	std::unique_ptr<UStringSetMap> as_set{ new UStringSetMap };

	for (auto rule : relabels.rule_by_number) {
		const TagVector& fromTags = trie_getTagList(rule->maplist->trie);
		Set* target = relabels.sets_list[rule->target];
		const TagVector& toTags = trie_getTagList(target->trie);
		if (!(rule->maplist->trie_special.empty() && target->trie_special.empty())) {
			u_fprintf(ux_stderr, "Warning: Relabel rule '%S' on line %d has %d special tags, skipping!\n", rule->name.c_str(), rule->line);
			continue;
		}
		if (!rule->tests.empty()) {
			u_fprintf(ux_stderr, "Warning: Relabel rule '%S' on line %d had context tests, skipping!\n", rule->name.c_str(), rule->line);
			continue;
		}
		if (rule->wordform) {
			u_fprintf(ux_stderr, "Warning: Relabel rule '%S' on line %d had a wordform, skipping!\n", rule->name.c_str(), rule->line);
			continue;
		}
		if (rule->type != K_MAP) {
			u_fprintf(ux_stderr, "Warning: Relabel rule '%S' on line %d has unexpected keyword (expected MAP), skipping!\n", rule->name.c_str(), rule->line);
			continue;
		}
		if (fromTags.size() != 1) {
			u_fprintf(ux_stderr, "Warning: Relabel rule '%S' on line %d has %d tags in the maplist (expected 1), skipping!\n", rule->name.c_str(), rule->line, fromTags.size());
			continue;
		}
		Tag* fromTag = fromTags[0];
		for (auto toit : toTags) {
			if (toit->type & T_SPECIAL) {
				u_fprintf(ux_stderr, "Warning: Special tags (%S) not supported yet.\n", toit->tag.c_str());
			}
		}
		if (!toTags.empty()) {
			as_list->emplace(fromTag->tag.c_str(), target);
		}
		else {
			as_set->emplace(fromTag->tag.c_str(), target);
		}
	}

	relabel_as_list = std::move(as_list);
	relabel_as_set = std::move(as_set);
}

TagVector Relabeller::transferTags(const TagVector tv_r) {
	TagVector tv_g;
	for (auto tag_r : tv_r) {
		Tag* tag_g = new Tag(*tag_r);
		tag_g = grammar->addTag(tag_g); // new is deleted if it exists
		tv_g.push_back(tag_g);
	}
	return tv_g;
}

// From TextualParser::parseTagList
struct freq_sorter {
	const bc::flat_map<Tag*, size_t>& tag_freq;

	freq_sorter(const bc::flat_map<Tag*, size_t>& tag_freq)
	  : tag_freq(tag_freq)
	{
	}

	bool operator()(Tag* a, Tag* b) const {
		// Sort highest frequency first
		return tag_freq.find(a)->second > tag_freq.find(b)->second;
	}
};
void Relabeller::addTaglistsToSet(const std::set<TagVector> tvs, Set* s) {
	// Extracted from TextualParser::parseTagList

	// Might be slightly faster to do this in relabelAsList after
	// transferTags, but seems clearer this way and compile speed
	// is fast enough

	if (tvs.empty()) {
		return;
	}

	bc::flat_map<Tag*, size_t> tag_freq;
	std::set<TagVector> tvs_sort_uniq;

	for (auto& tvc : tvs) {
		TagVector& tags = const_cast<TagVector&>(tvc);
		// From TextualParser::parseTagList
		std::sort(tags.begin(), tags.end());
		tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
		if (tvs_sort_uniq.insert(tags).second) {
			for (auto t : tags) {
				++tag_freq[t];
			}
		}
	}
	freq_sorter fs(tag_freq);
	for (auto& tvc : tvs_sort_uniq) {
		if (tvc.empty()) {
			continue;
		}
		if (tvc.size() == 1) {
			grammar->addTagToSet(tvc[0], s);
			continue;
		}
		TagVector& tv = const_cast<TagVector&>(tvc);
		// Sort tags by frequency, high-to-low
		// Doing this yields a very cheap imperfect form of trie compression, but it's good enough
		std::sort(tv.begin(), tv.end(), fs);
		bool special = false;
		for (auto tag : tv) {
			if (tag->type & T_SPECIAL) {
				special = true;
				break;
			}
		}
		if (special) {
			trie_insert(s->trie_special, tv);
		}
		else {
			trie_insert(s->trie, tv);
		}
	}
}

void Relabeller::relabelAsList(Set* set_g, const Set* set_r, const Tag* fromTag) {
	std::set<TagVector> old_tvs = trie_getTagsOrdered(set_g->trie);
	trie_delete(set_g->trie);
	set_g->trie.clear();

	std::set<TagVector> taglists;
	for (auto& old_tags : old_tvs) {
		TagVector tags_except_from;

		bool seen = false;
		for (auto old_tag : old_tags) {
			if (old_tag->hash == fromTag->hash) {
				seen = true;
			}
			else {
				tags_except_from.push_back(old_tag);
			}
		}
		std::set<TagVector> suffixes;
		if (seen) {
			suffixes = trie_getTagsOrdered(set_r->trie);
		}
		else {
			TagVector dummy;
			suffixes.insert(dummy);
		}
		for (auto& suf : suffixes) {
			TagVector tags = TagVector(tags_except_from);
			tags.insert(tags.end(), suf.begin(), suf.end());
			tags = transferTags(tags);
			taglists.insert(tags);
		}
	}
	addTaglistsToSet(taglists, set_g);
}

void Relabeller::reindexSet(Set& s) {
	s.type &= ~ST_SPECIAL;
	s.type &= ~ST_CHILD_UNIFY;

	s.type |= trie_reindex(s.trie);
	s.type |= trie_reindex(s.trie_special);

	for (uint32_t i = 0; i < s.sets.size(); ++i) {
		Set* set = grammar->sets_list[s.sets[i]];
		reindexSet(*set);
		if (set->type & ST_SPECIAL) {
			s.type |= ST_SPECIAL;
		}
		if (set->type & (ST_TAG_UNIFY | ST_SET_UNIFY | ST_CHILD_UNIFY)) {
			s.type |= ST_CHILD_UNIFY;
		}
		if (set->type & ST_MAPPING) {
			s.type |= ST_MAPPING;
		}
	}

	if (s.type & (ST_TAG_UNIFY | ST_SET_UNIFY | ST_CHILD_UNIFY)) {
		s.type |= ST_SPECIAL;
		s.type |= ST_CHILD_UNIFY;
	}
}

void Relabeller::addSetToGrammar(Set* s) {
	s->setName(grammar->sets_list.size() + 100);
	grammar->sets_list.push_back(s);
	s->number = (uint32_t)grammar->sets_list.size() - 1;
	reindexSet(*s);
}

uint32_t Relabeller::copyRelabelSetToGrammar(const Set* s_r) {
	Set* s_g = grammar->allocateSet();

	uint32_t nsets = s_r->sets.size();
	s_g->sets.resize(nsets);
	for (uint32_t i = 0; i < nsets; ++i) {
		// First ensure all referred-to sets exist:
		uint32_t child_num_r = s_r->sets[i];
		uint32_t child_num_g = copyRelabelSetToGrammar(relabels->sets_list[child_num_r]);
		s_g->sets[i] = child_num_g;
	}

	uint32_t nset_ops = s_r->set_ops.size();
	s_g->set_ops.resize(nset_ops);
	for (uint32_t i = 0; i < nset_ops; ++i) {
		s_g->set_ops[i] = s_r->set_ops[i]; // enum from Strings.cpp, same across grammars
	}

	s_g->trie = trie_copy(s_r->trie, *grammar);
	s_g->trie_special = trie_copy(s_r->trie_special, *grammar);
	s_g->ff_tags = s_r->ff_tags; // TODO: does this get copied correctly?
	addSetToGrammar(s_g);
	return s_g->number;
}

void Relabeller::relabelAsSet(Set* set_g, const Set* set_r, const Tag* fromTag) {
	if (set_g->trie.empty()) {
		// If the grammar's set is only an +/OR/- of other
		// sets, then we only need to change those other sets
		return;
	}
	if (!set_g->sets.empty()) {
		u_fprintf(ux_stderr, "Warning: SET %d has both trie and sets, this was unexpected.", set_g->number);
	}
	std::set<TagVector> old_tvs = trie_getTagsOrdered(set_g->trie);
	trie_delete(set_g->trie);
	set_g->trie.clear();
	// First we split old_tvs into those that contain fromTag, tvs_with_from, and those that don't, tvs_no_from
	// set_gW->trie = to_trie(tvs_with_from, but first removing fromTag)
	// set_gN->trie = to_trie(tvs_no_from)
	// Then we copy set_r to the relabelled grammar as set_gR
	// set_gI->sets = set_gR + set_gW
	// set_g->sets = set_gN OR set_gI
	// We also put the special and ff tags from set_g into set_gN

	std::set<TagVector> tvs_with_from;
	std::set<TagVector> tvs_no_from;
	for (auto& old_tags : old_tvs) {
		TagVector tags_except_from;

		bool seen = false;
		for (auto old_tag : old_tags) {
			if (old_tag->hash == fromTag->hash) {
				seen = true;
			}
			else {
				tags_except_from.push_back(old_tag);
			}
		}
		if (tags_except_from.empty()) {
			continue;
		}
		if (seen) {
			tvs_with_from.insert(transferTags(tags_except_from));
		}
		else {
			tvs_no_from.insert(transferTags(tags_except_from));
		}
	}
	Set* s_gN = grammar->allocateSet();
	addTaglistsToSet(tvs_no_from, s_gN);
	s_gN->trie_special = trie_copy(set_g->trie_special);
	s_gN->ff_tags = set_g->ff_tags;
	s_gN->sets = set_g->sets; // should be empty but who knows
	s_gN->set_ops = set_g->set_ops;
	addSetToGrammar(s_gN);

	uint32_t s_gR_num = copyRelabelSetToGrammar(set_r);
	uint32_t s_gI_num;
	if (tvs_with_from.empty()) {
		// We don't want to intersect with ∅, that would never match
		s_gI_num = s_gR_num;
	}
	else {
		Set* s_gW = grammar->allocateSet();
		addTaglistsToSet(tvs_with_from, s_gW);
		addSetToGrammar(s_gW);
		if (s_gW->getNonEmpty().empty()) {
			u_fprintf(ux_stderr, "Warning: unexpected empty tries when relabelling set %d!\n", set_g->number);
		}

		Set* s_gI = grammar->allocateSet(); // relabelling_of_fromTag + taglists_that_had_fromTag
		s_gI->sets.resize(2);
		s_gI->sets[0] = s_gR_num;
		s_gI->sets[1] = s_gW->number;
		s_gI->set_ops.resize(1);
		s_gI->set_ops[0] = S_PLUS;
		addSetToGrammar(s_gI);
		s_gI_num = s_gI->number;
	}

	set_g->sets.resize(2); // taglists_that_had_no_fromTag OR (relabelling_of_fromTag + taglists_that_had_no_fromTag)
	set_g->sets[0] = s_gN->number;
	set_g->sets[1] = s_gI_num;
	set_g->set_ops.resize(1);
	set_g->set_ops[0] = S_OR; // TODO: can avoid this if tvs_no_from.empty()
	reindexSet(*set_g);       // This one was already added to grammar
}

void Relabeller::relabel() {
	std::unordered_map<UString, Tag*, hash_ustring> tag_by_str;
	for (auto tag_g : grammar->single_tags_list) {
		tag_by_str[tag_g->tag] = tag_g;
	}
	std::unordered_map<UString, std::set<Set*>, hash_ustring> sets_by_tag;
	for (auto it : grammar->sets_list) {
		const TagVector& toTags = trie_getTagList(it->trie);
		for (auto toit : toTags) {
			sets_by_tag[toit->tag].insert(it);
		}
	}
	// RELABEL AS LIST:
	for (auto& it : *relabel_as_list) {
		const Set* set_r = relabels->sets_list[it.second->number];
		const Tag* fromTag = tag_by_str[it.first];

		const auto sets_g = sets_by_tag.find(it.first);
		if (sets_g != sets_by_tag.end()) {
			for (auto set_g : sets_g->second) {
				relabelAsList(set_g, set_r, fromTag);
			}
		}
	}
	// RELABEL AS SET:
	for (auto& it : *relabel_as_set) {
		const Set* set_r = relabels->sets_list[it.second->number];
		const Tag* fromTag = tag_by_str[it.first];

		const auto sets_g = sets_by_tag.find(it.first);
		if (sets_g != sets_by_tag.end()) {
			for (auto set_g : sets_g->second) {
				relabelAsSet(set_g, set_r, fromTag);
			}
		}
	}
	grammar->sets_by_tag.clear(); // need to re-add these, with the new sets_list.sizes
	grammar->reindex();
}
}
