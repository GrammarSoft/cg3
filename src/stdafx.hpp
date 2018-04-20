/*
* Copyright (C) 2007-2018, GrammarSoft ApS
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
	// warning C4512: assignment operator could not be generated
	#pragma warning (disable: 4512)
	// warning C4456: declaration hides previous local declaration
	#pragma warning (disable: 4456)
	// warning C4458: declaration hides class member
	#pragma warning (disable: 4458)
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
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <limits>
#include <ctime>
#include <cmath>
#include <climits>
#include <cassert>
#include <ciso646>
#include <sys/stat.h>
#include <cstdint>
#include <cycle.h>

// cycle.h doesn't know all platforms (such as ARM), so fall back on clock()
#ifndef HAVE_TICK_COUNTER
	typedef clock_t ticks;

	inline ticks getticks() {
		return clock();
	}

	INLINE_ELAPSED(inline);

	#define HAVE_TICK_COUNTER
#endif

#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/dynamic_bitset.hpp>

#define foreach(iter, container) \
	if (!(container).empty())    \
		for (auto iter = (container).begin(), iter##_end = (container).end(); iter != iter##_end; ++iter)

#define reverse_foreach(iter, container) \
	if (!(container).empty())            \
		for (auto iter = (container).rbegin(), iter##_end = (container).rend(); iter != iter##_end; ++iter)

#ifdef _WIN32
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
typedef std::vector<uint32_t> uint32Vector;
namespace bc = ::boost::container;
}

#include "inlines.hpp"
#include "uextras.hpp"
#include "flat_unordered_map.hpp"
#include "flat_unordered_set.hpp"

#endif
