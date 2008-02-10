/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
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

#include "uextras.h"

inline uint32_t hash_sdbm_uchar(const UChar *str, uint32_t hash) {
	if (hash == 0) {
		hash = 705577479U;
	}
    UChar c = 0;

	while ((c = *str++) != 0) {
        hash = c + (hash << 6U) + (hash << 16U) - hash;
	}

    return hash;
}

inline uint32_t hash_sdbm_char(const char *str, uint32_t hash) {
	if (hash == 0) {
		hash = 705577479U;
	}
    UChar c = 0;

	while ((c = *str++) != 0) {
        hash = c + (hash << 6U) + (hash << 16U) - hash;
	}

    return hash;
}

inline uint32_t hash_sdbm_uint32_t(const uint32_t c, uint32_t hash) {
	if (hash == 0) {
		hash = 705577479U;
	}
    hash = c + (hash << 6U) + (hash << 16U) - hash;
    return hash;
}

#ifdef WIN32
	#include <winsock.h> // for hton() and family.
    #include <hash_map>
    #include <hash_set>
#else
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
	typedef stdext::hash_set<uint32_t> uint32HashSet;
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
	class GrammarParser;
	class GrammarWriter;
	class CompositeTag;
	class ContextualTest;
}

#endif
