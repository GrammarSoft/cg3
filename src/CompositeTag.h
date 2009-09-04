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

#pragma once
#ifndef __COMPOSITETAG_H
#define __COMPOSITETAG_H

#include "stdafx.h"
#include "Tag.h"

namespace CG3 {

	class CompositeTag {
	public:
		uint32_t hash;
		uint32_t number;
		TagSet tags_set;
		TagHashSet tags;
		bool is_used;
		bool is_special;

		CompositeTag();
		~CompositeTag();

		void addTag(Tag *tag);

		uint32_t rehash();
		void markUsed();
	};

	struct compare_CompositeTag {
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;

		inline size_t operator() (const CompositeTag* x) const {
			return x->hash;
		}

		inline bool operator() (const CompositeTag* a, const CompositeTag* b) const {
			return a->hash < b->hash;
		}
	};

	typedef stdext::hash_set<CompositeTag*, compare_CompositeTag> CompositeTagHashSet;
}

#ifdef __GNUC__
#if GCC_VERSION < 40300
namespace __gnu_cxx {
	template<> struct hash< CG3::CompositeTag* > {
		size_t operator()( const CG3::CompositeTag *x ) const {
			return x->hash;
		}
	};
}
#endif
#endif

#endif
