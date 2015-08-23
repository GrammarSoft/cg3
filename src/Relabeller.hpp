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

// TODO: how's the uuid generated?
#pragma once
#ifndef b6d28b7452ec699b_RELABELLER_H
#define b6d28b7452ec699b_RELABELLER_H

#include "stdafx.hpp"
#include "TagTrie.hpp"
#include "Grammar.hpp"

namespace CG3 {
	class Tag;
	class Set;

	typedef stdext::hash_map<UString, UString > UStringMap;
	typedef stdext::hash_map<UString, Set* > UStringSetMap;

	class Relabeller {
	public:
		Relabeller(Grammar& res, const Grammar& relabels, UFILE *ux_err);
		~Relabeller();

		void relabel();

	private:
		UFILE *ux_stderr;
		Grammar *grammar;
		const Grammar *relabels;
		const UStringMap* relabel_as_tag;
		const UStringSetMap* relabel_as_list;
		const UStringSetMap* relabel_as_set;
		typedef stdext::hash_map<UString,uint32_t> set_id_map_t;

		typedef std::vector<Tag*> TagVector;
		bool needsSetOps(std::set<TagVector> tagsets[]);
		uint32_t addRelabelSet(Set* set);
		TagVector transferTags(const TagVector tv_r);
		void relabelAsList(Set* set_g, const Set* set_r, const Tag* fromTag);
		void relabelAsSet(Set* set_g, const Set* set_r, const Tag* fromTag);
	};

	inline trie_t *_trie_copy_helper(const trie_t& trie, Grammar& grammar) {
		trie_t *nt = new trie_t;
		boost_foreach (const trie_t::value_type& p, trie) {
			Tag* t = new Tag(*p.first);
			t->markUsed();
			grammar.single_tags_list.push_back(t);
			t->number = (uint32_t)grammar.single_tags_list.size()-1;
			(*nt)[t].terminal = p.second.terminal;
			if (p.second.trie) {
				(*nt)[t].trie = _trie_copy_helper(*p.second.trie);
			}
		}
		return nt;
	}

	inline trie_t trie_copy(const trie_t& trie, Grammar& grammar) {
		trie_t nt;
		boost_foreach (const trie_t::value_type& p, trie) {
			Tag* t = new Tag(*p.first);
			nt[t].terminal = p.second.terminal;
			if (p.second.trie) {
				nt[t].trie = _trie_copy_helper(*p.second.trie);
			}
		}
		return nt;
	}

	void printTrie(trie_t t, UFILE* out) {
		boost_foreach (const trie_t::value_type& kv, t) {
			u_fprintf(out, "(%S", kv.first->tag.c_str());
			if (kv.second.terminal) {
				u_fprintf(out, "!");
			}
			u_fprintf(out, " ");
			if (kv.second.trie) {
				printTrie(*kv.second.trie, out);
			}
			u_fprintf(out, ") ", kv.first->tag.c_str());
		}
	}

	// Destructively alters trie:
	inline void trie_append_copies(trie_t* trie, const bool terminal, const trie_t* suffix, UFILE* ux_stderr) {
		boost_foreach (trie_t::value_type& p, *trie) {
			if(p.second.trie) {
				trie_append_copies(p.second.trie, terminal, suffix, ux_stderr);
			}
			else {
				p.second.terminal = terminal;
				if(suffix) {
					p.second.trie = _trie_copy_helper(*suffix);
				}
			}
		}
	}

	// Destructively alters both trie and infix:
	inline void trie_splice(trie_t& trie, Tag* atTag, const trie_t& infix, UFILE* ux_stderr) {
		boost_foreach (trie_t::value_type& p, trie) {
			if(p.first->hash == atTag->hash) {
				trie_node_t old_node = p.second;
				trie_t* t_from_here = _trie_copy_helper(infix);
				trie_append_copies(t_from_here, old_node.terminal, old_node.trie, ux_stderr);
				trie.erase(p.first);
				// TODO: tags in infix might actually exist in suffix, or even in the path up to this node.
				// Instead of this method, we should first grab all possible tagvectors, splice in there, then sort+uniq them and re-insert.
				boost_foreach (trie_t::value_type& ph, *t_from_here) {
					trie[ph.first] = ph.second;
				}
			}
			else if(p.second.trie) {
				trie_splice(*p.second.trie, atTag, infix, ux_stderr);
			}
		}
	}
}

#endif
