/*
* Copyright (C) 2007-2011, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_RULE_H
#define c6d28b7452ec699b_RULE_H

#include "stdafx.h"
#include "Strings.h"
#include "Tag.h"
#include "Cohort.h"

namespace CG3 {

	class Grammar;
	class ContextualTest;
	class Set;

	// This must be kept in lock-step with Strings.h's FLAGS
	enum {
		RF_NEAREST      = (1 <<  0),
		RF_ALLOWLOOP    = (1 <<  1),
		RF_DELAYED      = (1 <<  2),
		RF_IMMEDIATE    = (1 <<  3),
		RF_LOOKDELETED  = (1 <<  4),
		RF_LOOKDELAYED  = (1 <<  5),
		RF_UNSAFE       = (1 <<  6),
		RF_SAFE         = (1 <<  7),
		RF_REMEMBERX    = (1 <<  8),
		RF_RESETX       = (1 <<  9),
		RF_KEEPORDER    = (1 << 10),
		RF_VARYORDER    = (1 << 11),
		RF_ENCL_INNER   = (1 << 12),
		RF_ENCL_OUTER   = (1 << 13),
		RF_ENCL_FINAL   = (1 << 14),
		RF_ENCL_ANY     = (1 << 15),
		RF_ALLOWCROSS   = (1 << 16),
		RF_WITHCHILD    = (1 << 17),
		RF_NOCHILD      = (1 << 18),
		RF_ITERATE      = (1 << 19),
		RF_NOITERATE    = (1 << 20),
		RF_UNMAPLAST    = (1 << 21),
	};

	class Rule {
	public:
		UChar *name;
		uint32_t wordform;
		uint32_t target;
		uint32_t childset1, childset2;
		uint32_t line, number;
		uint32_t varname, varvalue;
		uint32_t jumpstart, jumpend;
		uint32_t flags;
		int32_t section;
		// ToDo: Add proper "quality" quantifier based on num_fail, num_match, total_time
		double weight, quality;
		KEYWORDS type;
		Set *maplist;
		Set *sublist;
		
		mutable ContextualTest *test_head;
		mutable ContextualTest *dep_test_head;
		mutable uint32_t num_fail, num_match;
		mutable double total_time;
		mutable ContextualTest *dep_target;

		Rule();
		~Rule();
		void setName(const UChar *to);
		
		void resetStatistics();

		ContextualTest *allocateContextualTest();
		void addContextualTest(ContextualTest *to, ContextualTest **head);
		void reverseContextualTests();

		static bool cmp_quality(const Rule *a, const Rule *b);

		static inline size_t cmp_hash(const Rule* r) {
			return hash_sdbm_uint32_t(r->number);
		}
		static inline bool cmp_compare(const Rule* a, const Rule* b) {
			return a->number < b->number;
		}
	};

	struct compare_Rule {
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;

		inline size_t operator() (const Rule* r) const {
			return Rule::cmp_hash(r);
		}

		inline bool operator() (const Rule* a, const Rule* b) const {
			return Rule::cmp_compare(a, b);
		}
	};

	typedef std::vector<Rule*> RuleVector;
	typedef std::map<uint32_t,Rule*> RuleByLineMap;
	typedef stdext::hash_map<uint32_t,Rule*> RuleByLineHashMap;
	typedef stdext::hash_map<const Rule*, CohortSet, compare_Rule> RuleToCohortsMap;
}

#ifdef __GNUC__
#ifndef HAVE_BOOST
#if GCC_VERSION < 40300
namespace __gnu_cxx {
	template<> struct hash< CG3::Rule* > {
		size_t operator()( const CG3::Rule *x ) const {
			return CG3::Rule::cmp_hash(x);
		}
	};
}
#endif
#endif
#endif

#endif
