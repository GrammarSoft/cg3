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
#ifndef b6d28b7452ec699b_RELABELLER_H
#define b6d28b7452ec699b_RELABELLER_H

#include "stdafx.hpp"
#include "TagTrie.hpp"
#include "Grammar.hpp"

namespace CG3 {
class Tag;
class Set;

class Relabeller {
public:
	Relabeller(Grammar& res, const Grammar& relabels, std::ostream& ux_err);

	void relabel();

private:
	std::ostream* ux_stderr;
	Grammar* grammar;
	const Grammar* relabels;

	typedef std::unordered_map<UString, UString, hash_ustring> UStringMap;
	typedef std::unordered_map<UString, Set*, hash_ustring> UStringSetMap;
	std::unique_ptr<const UStringSetMap> relabel_as_list;
	std::unique_ptr<const UStringSetMap> relabel_as_set;

	typedef std::vector<Tag*> TagVector;
	uint32_t copyRelabelSetToGrammar(const Set* set);
	TagVector transferTags(const TagVector tv_r);
	void addTaglistsToSet(const std::set<TagVector> tvs, Set* set);
	void reindexSet(Set& s);
	void addSetToGrammar(Set* s);
	void relabelAsList(Set* set_g, const Set* set_r, const Tag* fromTag);
	void relabelAsSet(Set* set_g, const Set* set_r, const Tag* fromTag);
};

inline trie_t* _trie_copy_helper(const trie_t& trie, Grammar& grammar) {
	trie_t* nt = new trie_t;
	for (auto& p : trie) {
		Tag* t = new Tag(*p.first);
		t = grammar.addTag(t); // new is deleted if it exists
		(*nt)[t].terminal = p.second.terminal;
		if (p.second.trie) {
			(*nt)[t].trie = _trie_copy_helper(*p.second.trie);
		}
	}
	return nt;
}

inline trie_t trie_copy(const trie_t& trie, Grammar& grammar) {
	trie_t nt;
	for (auto& p : trie) {
		Tag* t = new Tag(*p.first);
		t = grammar.addTag(t); // new is deleted if it exists
		nt[t].terminal = p.second.terminal;
		if (p.second.trie) {
			nt[t].trie = _trie_copy_helper(*p.second.trie);
		}
	}
	return nt;
}
}

#endif
