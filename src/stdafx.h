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
#ifndef c6d28b7452ec699b_STDAFX_H
#define c6d28b7452ec699b_STDAFX_H

#ifdef _MSC_VER
	#define _SECURE_SCL 0
	#define _ITERATOR_DEBUG_LEVEL 0
	#define _CRT_SECURE_NO_DEPRECATE
	#define WIN32_LEAN_AND_MEAN
	#define VC_EXTRALEAN
	#define NOMINMAX
	// warning C4428: universal-character-name encountered in source
	#pragma warning (disable: 4428)
#endif

#include <exception>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <ostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <list>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <stack>
#include <limits>
#include <ctime>
#include <cmath>
#include <climits>
#include <cassert>
#include <ciso646>
#include <sys/stat.h>
#include <stdint.h> // C99 or C++0x or C++ TR1 will have this header. ToDo: Change to <cstdint> when C++0x broader support gets under way.

#ifdef _MSC_VER
	#ifdef HAVE_BOOST
		#include <boost/unordered_set.hpp>
		#include <boost/unordered_map.hpp>
		#define stdext boost
		#define hash_map unordered_map
		#define hash_set unordered_set
		#define hash_multimap unordered_multimap
		#define hash_multiset unordered_multiset
	// Test for MSVC++ >= 9.0 (MSVS 2008)
	#elif _MSC_VER >= 1500
		#include <unordered_set>
		#include <unordered_map>
		#define stdext std::tr1
		#define hash_map unordered_map
		#define hash_set unordered_set
		#define hash_multimap unordered_multimap
		#define hash_multiset unordered_multiset
	#else
		#include <hash_map>
		#include <hash_set>
	#endif
	#include <winsock.h> // for hton() and family.
#elif defined(__INTEL_COMPILER)
	#include <ext/hash_map>
	#include <ext/hash_set>
	#define stdext __gnu_cxx
	#include <unistd.h>
	#include <libgen.h>
	#include <netinet/in.h> // for hton() and family.
#elif defined(__GNUC__)
	#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
	#ifdef HAVE_BOOST
		#include <boost/unordered_set.hpp>
		#include <boost/unordered_map.hpp>
		#define stdext boost
		#define hash_map unordered_map
		#define hash_set unordered_set
		#define hash_multimap unordered_multimap
		#define hash_multiset unordered_multiset
	// Test for GCC >= 4.3.0
	#elif GCC_VERSION >= 40300
		#include <tr1/unordered_set>
		#include <tr1/unordered_map>
		#define stdext std::tr1
		#define hash_map unordered_map
		#define hash_set unordered_set
		#define hash_multimap unordered_multimap
		#define hash_multiset unordered_multiset
	#else
		#include <ext/hash_map>
		#include <ext/hash_set>
		#define stdext __gnu_cxx
	#endif
	#include <unistd.h>
	#include <libgen.h>
	#include <netinet/in.h> // for hton() and family.
#else
	#error "Unknown compiler...please customize stdafx.h for it."
#endif

// ICU includes
#include <unicode/unistr.h>
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
#include <unicode/ubrk.h>

namespace CG3 {
	typedef std::basic_string<UChar> UString;
	typedef std::list<uint32_t> uint32List;
	typedef std::vector<uint32_t> uint32Vector;
	typedef std::set<uint32_t> uint32Set;
	typedef std::map<uint32_t,int32_t> uint32int32Map;
	typedef std::map<uint32_t,uint32_t> uint32Map;
	typedef stdext::hash_set<uint32_t> uint32HashSet;
	typedef stdext::hash_map<uint32_t,uint32_t> uint32HashMap;
	typedef stdext::hash_map<uint32_t,uint32Set> uint32Setuint32HashMap;
	typedef stdext::hash_map<uint32_t,uint32HashSet> uint32HashSetuint32HashMap;
}

#include "cycle.h"
#include "macros.h"
#include "inlines.h"
#include "uextras.h"

#endif
