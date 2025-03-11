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
#ifndef c6d28b7452ec699b_TAG_H
#define c6d28b7452ec699b_TAG_H

#include "stdafx.hpp"
#include "sorted_vector.hpp"
#include "flat_unordered_map.hpp"

namespace CG3 {
class Grammar;
class Set;

using SetVector = std::vector<Set*>;

enum C_OPS : uint8_t {
	OP_NOP,
	OP_EQUALS,
	OP_LESSTHAN,
	OP_GREATERTHAN,
	OP_LESSEQUALS,
	OP_GREATEREQUALS,
	OP_NOTEQUALS,
	NUM_OPS,
};

enum : uint32_t {
	T_ANY              = (1 <<  0),
	T_NUMERICAL        = (1 <<  1),
	T_MAPPING          = (1 <<  2),
	T_VARIABLE         = (1 <<  3),
	T_META             = (1 <<  4),
	T_WORDFORM         = (1 <<  5),
	T_BASEFORM         = (1 <<  6),
	T_TEXTUAL          = (1 <<  7),
	T_DEPENDENCY       = (1 <<  8),
	T_SAME_BASIC       = (1 <<  9),
	T_FAILFAST         = (1 << 10),
	T_CASE_INSENSITIVE = (1 << 11),
	T_REGEXP           = (1 << 12),
	T_PAR_LEFT         = (1 << 13),
	T_PAR_RIGHT        = (1 << 14),
	T_REGEXP_ANY       = (1 << 15),
	T_VARSTRING        = (1 << 16),
	T_TARGET           = (1 << 17),
	T_MARK             = (1 << 18),
	T_ATTACHTO         = (1 << 19),
	T_SPECIAL          = (1 << 20),
	T_USED             = (1 << 21),
	T_LOCAL_VARIABLE   = (1 << 22),
	T_SET              = (1 << 23),
	T_VSTR             = (1 << 24),
	T_ENCL             = (1 << 25),
	T_RELATION         = (1 << 26),
	T_CONTEXT          = (1 << 27),
	T_NUMERIC_MATH     = (1 << 28),

	T_REGEXP_LINE      = (1u << 31), // ToDo: Remove for real ordered mode

	MASK_TAG_SPECIAL   = T_ANY | T_TARGET | T_MARK | T_ATTACHTO | T_PAR_LEFT | T_PAR_RIGHT | T_NUMERICAL | T_VARIABLE | T_LOCAL_VARIABLE | T_META | T_FAILFAST | T_CASE_INSENSITIVE | T_REGEXP | T_REGEXP_ANY | T_VARSTRING | T_SET | T_ENCL | T_SAME_BASIC | T_CONTEXT,
};

class Tag {
public:
	CG3_IMPORTS static std::ostream* dump_hashes_out;

	C_OPS comparison_op = OP_NOP;
	double comparison_val = 0;
	uint32_t type = 0;
	uint32_t comparison_hash = 0;
	uint32_t dep_self = 0;
	union {
		uint32_t dep_parent = 0;
		uint32_t variable_hash;
		uint32_t context_ref_pos;
		uint32_t comparison_offset;
	};
	uint32_t hash = 0;
	uint32_t plain_hash = 0;
	uint32_t number = 0;
	uint32_t seed = 0;
	UString tag;
	UString tag_raw;
	std::unique_ptr<SetVector> vs_sets;
	std::unique_ptr<UStringVector> vs_names;
	mutable URegularExpression* regexp = nullptr;

	Tag() = default;
	Tag(const Tag& o);
	~Tag();
	void parseTagRaw(const UChar* to, Grammar* grammar);
	UString toUString(bool escape = false) const;

	uint32_t rehash();
	void markUsed();
	void allocateVsSets();
	void allocateVsNames();
	void parseNumeric(bool trusted = false);
};

struct compare_Tag {
	inline bool operator()(const Tag* a, const Tag* b) const {
		return a->hash < b->hash;
	}
};

struct equal_Tag {
	inline bool operator()(const Tag* a, const Tag* b) const {
		return a->hash == b->hash;
	}
};

using TagVector = std::vector<Tag*>;
using TagList = TagVector;
using Taguint32HashMap = flat_unordered_map<uint32_t, Tag*>;
using TagSortedVector = sorted_vector<Tag*, compare_Tag>;

struct compare_TagVector {
	inline bool operator()(const TagVector& a, const TagVector& b) const {
		for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
			if (a[i]->hash != b[i]->hash) {
				return a[i]->hash < b[i]->hash;
			}
		}

		return a.size() < b.size();
	}
};

using TagVectorSet = std::set<TagVector, compare_TagVector>;

template<typename T>
inline void fill_tagvector(const T& in, TagVector& tags, bool& did, bool& special) {
	for (auto tag : in) {
		if (tag->type & T_NUMERICAL) {
			did = true;
		}
		else {
			if (tag->type & T_SPECIAL) {
				special = true;
			}
			tags.push_back(tag);
		}
	}
}
}

#endif
