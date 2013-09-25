/*
* Copyright (C) 2007-2013, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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
	// warning C4428: universal-character-name encountered in source
	#pragma warning (disable: 4428)
	// warning C4512: assignment operator could not be generated
	#pragma warning (disable: 4512)
	// warning C4480: nonstandard extension used: specifying underlying type for enum
	// 'cause that is actually standard in C++11
	#pragma warning (disable: 4480)
#endif

#include <exception>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <memory>
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
#include <cycle.h>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/scoped_ptr.hpp>
#define stdext boost
#define hash_map unordered_map
#define hash_set unordered_set
#define hash_multimap unordered_multimap
#define hash_multiset unordered_multiset

#ifdef _MSC_VER
	#include <winsock.h> // for hton() and family.
#else
	#include <unistd.h>
	#include <libgen.h>
	#include <netinet/in.h> // for hton() and family.
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
	typedef std::vector<UString> UStringVector;
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

#include "macros.hpp"
#include "inlines.hpp"
#include "uextras.hpp"

#endif