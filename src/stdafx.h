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

#ifndef __STDAFX_H
#define __STDAFX_H

// MSVC 2005 (MSVC 8) fix.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <bitset>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <assert.h>

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

void CG3Quit(const int32_t c, const char* file = 0, const uint32_t line = 0);

#include "macros.h"
#include "inlines.h"
#include "uextras.h"

#ifdef WIN32
	#include <winsock.h> // for hton() and family.
    #include <hash_map>
    #include <hash_set>
#endif
#ifdef __GNUC__
	#include <netinet/in.h> // for hton() and family.
    #include <ext/hash_map>
    #include <ext/hash_set>
    #define stdext __gnu_cxx
#endif

#ifndef MAX
	#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
	#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#endif

// Forward declarations
namespace CG3 {
	typedef std::list<uint32_t> uint32List;
	typedef std::vector<uint32_t> uint32Vector;
	typedef std::set<uint32_t> uint32Set;
	typedef std::map<uint32_t, uint32_t> uint32Map;
	typedef stdext::hash_set<uint32_t> uint32HashSet;
	typedef stdext::hash_map<uint32_t, uint32_t> uint32HashMap;
	class Recycler;
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
	typedef std::list<Tag*> TagList;
	typedef std::set<Tag*> TagSet;
	typedef stdext::hash_set<Tag*> TagHashSet;
	typedef stdext::hash_set<CompositeTag*> CompositeTagHashSet;
	typedef stdext::hash_map<uint32_t,Set*> uint32SetHashMap;
	typedef std::map<uint32_t, Rule*> RuleByLineMap;
	typedef std::vector<Rule*> RuleVector;
}

#endif
