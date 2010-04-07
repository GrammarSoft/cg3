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
#ifndef __TAG_H
#define __TAG_H

#include "stdafx.h"

namespace CG3 {

	enum C_OPS {
		OP_NOP,
		OP_EQUALS,
		OP_LESSTHAN,
		OP_GREATERTHAN,
		OP_LESSEQUALS,
		OP_GREATEREQUALS,
		OP_NOTEQUALS,
		NUM_OPS
	};

	enum TAG_TYPE {
		T_ANY        =      1U,
		T_NUMERICAL  =      2U,
		T_MAPPING    =      4U,
		T_VARIABLE   =      8U,
		T_META       =     16U,
		T_WORDFORM   =     32U,
		T_BASEFORM   =     64U,
		T_TEXTUAL    =    128U,
		T_DEPENDENCY =    256U,
		T_NEGATIVE   =    512U,
		T_FAILFAST   =   1024U,
		T_CASE_INSENSITIVE = 2048U,
		T_REGEXP     =   4096U,
		T_PAR_LEFT   =   8192U,
		T_PAR_RIGHT  =  16384U,
		T_REGEXP_ANY =  32768U,
		T_VARSTRING  =  65536U,
		T_TARGET     = 131072U,
		T_MARK       = 262144U,
		T_ATTACHTO   = 524288U,
	};

	class Tag {
	public:
		static bool dump_hashes;
		static UFILE* dump_hashes_out;

		bool in_grammar;
		bool is_special;
		bool is_used;
		C_OPS comparison_op;
		int32_t comparison_val;
		uint32_t type;
		uint32_t comparison_hash;
		uint32_t dep_self, dep_parent;
		uint32_t hash;
		uint32_t plain_hash;
		uint32_t number;
		uint32_t seed;
		UChar *comparison_key;
		UChar *tag;
		mutable URegularExpression *regexp;

		Tag();
		~Tag();
		UChar *allocateUChars(uint32_t n);
		void parseTag(const UChar *to, UFILE *ux_stderr);
		void parseTagRaw(const UChar *to);
		UString toUString() const;
		static void parseNumeric(Tag *tag, const UChar *txt);

		uint32_t rehash();
		void markUsed();
	};

	struct compare_Tag {
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;

		inline size_t operator() (const Tag* x) const {
			return x->hash;
		}

		inline bool operator() (const Tag* a, const Tag* b) const {
			return a->hash < b->hash;
		}
	};

	typedef std::list<Tag*> TagList;
	typedef std::vector<Tag*> TagVector;
	typedef std::set<Tag*, compare_Tag> TagSet;
	typedef stdext::hash_map<uint32_t,Tag*> Taguint32HashMap;
	typedef stdext::hash_set<Tag*, compare_Tag> TagHashSet;
}

#ifdef __GNUC__
#ifndef HAVE_BOOST
#if GCC_VERSION < 40300
namespace __gnu_cxx {
	template<> struct hash< CG3::Tag* > {
		size_t operator()( const CG3::Tag *x ) const {
			return x->hash;
		}
	};
}
#endif
#endif
#endif

#endif
