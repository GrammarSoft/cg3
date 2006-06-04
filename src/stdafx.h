#ifndef __STDAFX_H
#define __STDAFX_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ICU includes
#include <unicode/uclean.h>
#include <unicode/ustdio.h>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/utrans.h>
#include <unicode/ustring.h>

// CG3 includes
#include "cg3_resources.h"
#include "cg3_structs.h"
#include "hashtable.h"

/*
#include "tests.h"
// hash_map fix for cross-platform
#ifdef WIN32
    #include <hash_map>
#else
    #include <ext/hash_map>
    #define stdext __gnu_cxx
    namespace __gnu_cxx {
        template<> struct hash<std::string>
        {
            size_t operator()(const std::string& s) const {
                return __gnu_cxx::__stl_hash_string(s.c_str());
            }
        };
    }
#endif
//*/

#endif
