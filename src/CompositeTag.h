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
#ifndef c6d28b7452ec699b_COMPOSITETAG_H
#define c6d28b7452ec699b_COMPOSITETAG_H

#include "stdafx.h"
#include "Tag.h"

namespace CG3 {

	class CompositeTag {
	public:
		bool is_used;
		bool is_special;
		uint32_t hash;
		uint32_t number;
		TagSet tags_set;
		TagList tags;

		CompositeTag();

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

	enum ANYTAG_TYPE {
		ANYTAG_TAG = 0,
		ANYTAG_COMPOSITE,
		NUM_ANYTAG
	};

	class AnyTag {
	public:
		uint8_t which;

		AnyTag(Tag *tag) :
		which(ANYTAG_TAG),
		tag(tag)
		{
		}

		AnyTag(CompositeTag *tag) :
		which(ANYTAG_COMPOSITE),
		tag(tag)
		{
		}

		Tag *getTag() const {
			return static_cast<Tag*>(tag);
		}

		CompositeTag *getCompositeTag() const {
			return static_cast<CompositeTag*>(tag);
		}

		uint32_t hash() const {
			if (which == ANYTAG_TAG) {
				return getTag()->hash;
			}
			return getCompositeTag()->hash;
		}

		uint32_t number() const {
			if (which == ANYTAG_TAG) {
				return getTag()->number;
			}
			return getCompositeTag()->number;
		}

		bool operator<(const AnyTag& o) const {
			return tag < o.tag;
		}

	private:
		void *tag;
	};

	typedef stdext::hash_set<CompositeTag*, compare_CompositeTag> CompositeTagHashSet;
	typedef std::vector<AnyTag> AnyTagVector;
	typedef std::list<AnyTag> AnyTagList;
	typedef std::set<AnyTag> AnyTagSet;
}

#ifdef __GNUC__
#ifndef HAVE_BOOST
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

#endif
