/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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

#pragma once
#ifndef __SET_H
#define __SET_H

#include "stdafx.h"
#include "CompositeTag.h"
#include "miniset.h"

namespace CG3 {
	class Grammar;

	class Set {
	public:
		static bool dump_hashes;
		static UFILE* dump_hashes_out;

		bool match_any;
		bool is_special;
		bool is_tag_unified;
		bool is_set_unified;
		bool is_child_unified;
		bool is_used;
		uint32_t line;
		uint32_t hash;
		uint32_t number;
		mutable uint32_t num_fail, num_match;
		mutable double total_time;
		UString name;

		uint32Set tags_set;
		CompositeTagHashSet tags;
		TagHashSet single_tags;
		uint32MiniSet single_tags_hash;
		TagHashSet ff_tags;
		uint32MiniSet ff_tags_hash;

		uint32Vector set_ops;
		uint32Vector sets;

		Set();

		void setName(uint32_t to = 0);
		void setName(const UChar *to);

		uint32_t rehash();
		void resetStatistics();
		void reindex(Grammar &grammar);
		void markUsed(Grammar &grammar);
	};

	struct compare_Set {
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;

		inline size_t operator() (const Set* x) const {
			return x->hash;
		}

		inline bool operator() (const Set* a, const Set* b) const {
			return a->hash < b->hash;
		}
	};

	typedef std::set<Set*> SetSet;
	typedef stdext::hash_map<uint32_t,Set*> Setuint32HashMap;
}

#endif
