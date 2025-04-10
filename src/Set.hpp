/*
* Copyright (C) 2007-2025, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_SET_H
#define c6d28b7452ec699b_SET_H

#include "stdafx.hpp"
#include "TagTrie.hpp"
#include "sorted_vector.hpp"

namespace CG3 {
class Grammar;

enum {
	ST_ANY         = (1 <<  0),
	ST_SPECIAL     = (1 <<  1),
	ST_TAG_UNIFY   = (1 <<  2),
	ST_SET_UNIFY   = (1 <<  3),
	ST_CHILD_UNIFY = (1 <<  4),
	ST_MAPPING     = (1 <<  5),
	ST_USED        = (1 <<  6),
	ST_STATIC      = (1 <<  7),
	ST_ORDERED     = (1 <<  8),

	MASK_ST_UNIFY  = ST_TAG_UNIFY | ST_SET_UNIFY | ST_CHILD_UNIFY,
};

class Set {
public:
	CG3_IMPORTS static std::ostream* dump_hashes_out;

	uint16_t type = 0; // Stored as 32bit in binary format, so safe to bump when needed
	uint32_t line = 0;
	uint32_t hash = 0;
	uint32_t number = 0;
	UString name;

	trie_t trie;
	trie_t trie_special;
	TagSortedVector ff_tags;

	uint32Vector set_ops;
	uint32Vector sets;

	Set() = default;
	~Set() {
		trie_delete(trie);
		trie_delete(trie_special);
	}

	void setName(uint32_t to = 0);
	void setName(const UChar* to);
	void setName(const UString& to);
	void setName(const UStringView& to) {
		return setName(to.data());
	}

	bool empty() const;
	uint32_t rehash();
	void reindex(Grammar& grammar);
	void markUsed(Grammar& grammar);

	trie_t& getNonEmpty() {
		if (!trie.empty()) {
			return trie;
		}
		return trie_special;
	}
};

typedef sorted_vector<Set*> SetSet;
typedef std::vector<Set*> SetVector;
typedef std::unordered_map<uint32_t, Set*> Setuint32HashMap;

inline uint8_t trie_reindex(const trie_t& trie) {
	uint8_t type = 0;
	for (auto& kv : trie) {
		if (kv.first->type & T_SPECIAL) {
			type |= ST_SPECIAL;
		}
		if (kv.first->type & T_MAPPING) {
			type |= ST_MAPPING;
		}
		if (kv.second.trie) {
			type |= trie_reindex(*kv.second.trie);
		}
	}
	return type;
}
}

#endif
