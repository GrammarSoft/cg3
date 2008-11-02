/*
* Copyright (C) 2007, GrammarSoft ApS
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

#ifndef __TAG_H
#define __TAG_H

#include "stdafx.h"
#include "Strings.h"

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
		T_ANY        =  1U,
		T_NUMERICAL  =  2U,
		T_MAPPING    =  4U,
		T_VARIABLE   =  8U,
		T_META       = 16U,
		T_WORDFORM   = 32U,
		T_BASEFORM   = 64U,
		T_TEXTUAL    = 128U,
		T_DEPENDENCY = 256U,
		T_NEGATIVE   =  512U,
		T_FAILFAST   =  1024U,
		T_CASE_INSENSITIVE = 2048U,
		T_REGEXP     =  4096U,
		T_PAR_LEFT   =  8192U,
		T_PAR_RIGHT  = 16384U,
		T_REGEXP_ANY = 32768U
	};

	class Tag {
	public:
		uint32_t type;
		mutable URegularExpression *regexp;

		bool in_grammar;
		bool is_special;
		bool is_used;
		uint32_t comparison_hash;
		UChar *comparison_key;
		C_OPS comparison_op;
		int32_t comparison_val;
		uint32_t dep_self, dep_parent;
		UChar *tag;
		uint32_t hash;
		uint32_t number;

		Tag();
		~Tag();
		UChar *allocateUChars(uint32_t n);
		void parseTag(const UChar *to, UFILE *ux_stderr);
		void parseTagRaw(const UChar *to);
		static void parseNumeric(Tag *tag, const UChar *txt);
		static void printTagRaw(UFILE *out, const Tag *tag);

		uint32_t rehash();
		void markUsed();
	};

}

#ifdef __GNUC__
namespace __gnu_cxx {
	template<> struct hash< CG3::Tag* > {
		size_t operator()( const CG3::Tag *x ) const {
			return x->hash;
		}
	};
}
#endif

#endif
