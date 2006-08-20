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

// ICU includes
#include <unicode/uclean.h>
#include <unicode/ustdio.h>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/utrans.h>
#include <unicode/ustring.h>

//*
// hash_map fix for cross-platform
unsigned long hash_sdbm_char(const unsigned char *str);
unsigned long hash_sdbm_uchar(const UChar *str);

#ifdef WIN32
	#include <winsock.h>
    #include <hash_map>
/*
	struct uchar_hash {
		const static size_t bucket_size = sizeof(UChar*);
		const static size_t min_buckets = 3;

		size_t operator()(const UChar* str) const {
			size_t uhash = 0;
			int c;

			while (c = *str++) {
				uhash = c + (uhash << 6) + (uhash << 16) - uhash;
			}

			return uhash;
		}
	};

	struct uchar_equal {
		const static size_t bucket_size = sizeof(UChar*);
		const static size_t min_buckets = 3;

		size_t operator()(const UChar* stra, const UChar* strb) const {
			return (stra == strb) || u_strcmp(stra, strb) == 0;
		}
	};
//*/
#else
	#include <netinet/in.h>
    #include <ext/hash_map>
    #define stdext __gnu_cxx
    namespace __gnu_cxx {
        template<> struct hash<std::string> {
            size_t operator()(const std::string& s) const {
                return __gnu_cxx::__stl_hash_string(s.c_str());
            }
        };
        template<> struct hash<UChar*> {
            size_t operator()(const UChar *str) const {
                size_t uhash = 0;
                int c;

                while (c = *str++) {
                    uhash = c + (uhash << 6) + (uhash << 16) - uhash;
                }

                return uhash;
            }
        };
    }
#endif
//*/

// CG3 includes
#include "cg3_resources.h"

#endif
