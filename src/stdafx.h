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
#ifndef __STDAFX_H
#define __STDAFX_H

#ifdef _MSC_VER
	// MSVC 2005 (MSVC 8) fix.
	#define _SECURE_SCL 0
	#define _CRT_SECURE_NO_DEPRECATE 1
	#define _CRT_NONSTDC_NO_DEPRECATE 1
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <winsock.h> // for hton() and family.
	#include <hash_map>
	#include <hash_set>
#endif
#ifdef __GNUC__
	#include <unistd.h>
	#include <libgen.h>
	#include <netinet/in.h> // for hton() and family.
	// Test for GCC >= 4.3.0
	#if __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ >= 0)))
		#include <tr1/unordered_set>
		#include <tr1/unordered_map>
		#define stdext::hash_map std::tr1::unordered_map
		#define stdext::hash_set std::tr1::unordered_set
	#else
		#include <ext/hash_map>
		#include <ext/hash_set>
		#define stdext __gnu_cxx
	#endif
#endif

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <bitset>
#include <ctime>
#include <cmath>
#include <climits>
#include <cassert>
#include <sys/stat.h>

// ICU includes
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/ustdio.h>
#include <unicode/utypes.h>
#include <unicode/uloc.h>
#include <unicode/uenum.h>
#include <unicode/ucnv.h>
#include <unicode/utrans.h>
#include <unicode/ustring.h>
#include <unicode/uregex.h>

#include "cycle.h"
#include "macros.h"
#include "inlines.h"
#include "uextras.h"

// Forward declarations
namespace CG3 {
	typedef std::list<uint32_t> uint32List;
	typedef std::vector<uint32_t> uint32Vector;
	typedef std::set<uint32_t> uint32Set;
	typedef std::map<uint32_t,int32_t> uint32int32Map;
	typedef std::map<uint32_t,uint32_t> uint32Map;
	typedef std::map<uint32_t,uint32Set*> uint32Setuint32Map;
	typedef stdext::hash_set<uint32_t> uint32HashSet;
	typedef stdext::hash_map<uint32_t,uint32_t> uint32HashMap;
	typedef stdext::hash_map<uint32_t,uint32Set*> uint32Setuint32HashMap;
	typedef stdext::hash_map<uint32_t,uint32HashSet*> uint32HashSetuint32HashMap;

	class Grammar;
	class Set;
	class Rule;
	class Cohort;
	class Anchor;
	class Tag;
	class Window;
	class SingleWindow;
	class Reading;
	class GrammarApplicator;
	class GrammarWriter;
	class CompositeTag;
	class ContextualTest;

	struct compare_Tag;
	struct compare_CompositeTag;
	struct compare_Set;
	struct compare_Cohort;
	struct compare_Rule;

	typedef std::list<Tag*> TagList;
	typedef std::vector<Rule*> RuleVector;
	typedef std::vector<Tag*> TagVector;
	typedef std::set<Cohort*, compare_Cohort> CohortSet;
	typedef std::set<Tag*, compare_Tag> TagSet;
	typedef std::set<Set*> SetSet;
	typedef std::map<uint32_t,Rule*> RuleByLineMap;
	typedef stdext::hash_set<Tag*, compare_Tag> TagHashSet;
	typedef stdext::hash_set<CompositeTag*, compare_CompositeTag> CompositeTagHashSet;
	typedef stdext::hash_map<uint32_t,Rule*> RuleByLineHashMap;
	typedef stdext::hash_map<uint32_t,Set*> Setuint32HashMap;
	typedef stdext::hash_map<uint32_t,Tag*> Taguint32HashMap;
	typedef stdext::hash_map<const Rule*, CohortSet*, compare_Rule> RuleToCohortsMap;
}

#endif
