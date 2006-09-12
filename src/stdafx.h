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

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <time.h>

// ICU includes
#include <unicode/uclean.h>
#include <unicode/ustdio.h>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/utrans.h>
#include <unicode/ustring.h>

#include "uextras.h"

/*
// hash_map fix for cross-platform
uint32_t hash_sdbm_uchar(const UChar *str, uint32_t hash);
//*/
__inline uint32_t hash_sdbm_uchar(const UChar *str, uint32_t hash) {
    UChar c = 0;

	while ((c = *str++) != 0) {
        hash = c + (hash << 6) + (hash << 16) - hash;
	}

    return hash;
}

__inline uint32_t hash_sdbm_char(const char *str, uint32_t hash) {
    UChar c = 0;

	while ((c = *str++) != 0) {
        hash = c + (hash << 6) + (hash << 16) - hash;
	}

    return hash;
}

__inline uint32_t hash_sdbm_uint32_t(uint32_t c, uint32_t hash) {
    hash = c + (hash << 6) + (hash << 16) - hash;
    return hash;
}

#ifdef WIN32
	#include <winsock.h> // for hton() and family.
    #include <hash_map>
#else
	#include <netinet/in.h> // for hton() and family.
    #include <ext/hash_map>
    #define stdext __gnu_cxx
/*
    namespace __gnu_cxx {
        template<> struct hash<UChar*> {
            size_t operator()(const UChar *str) const {
                return hash_sdbm_uchar(str, 0);
            }
        };
    }
//*/
#endif

// CG3 includes
#include "cg3_resources.h"

#ifdef _DEBUG
extern uint32_t _np_ux_trim, _np_ux_findMatchingParenthesis;
#endif

#endif
