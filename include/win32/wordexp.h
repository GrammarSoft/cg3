#ifndef c6d28b7452ec699b_WORDEXP_H
#define c6d28b7452ec699b_WORDEXP_H

#include <stdafx.hpp>
#include <stdio.h>
#include <string.h>

namespace CG3_WordExp {

struct wordexp_t {
	size_t we_wordc;
	char** we_wordv;
	size_t we_offs;
};

constexpr int WRDE_DOOFFS = 0x0001;
constexpr int WRDE_APPEND = 0x0002;
constexpr int WRDE_NOCMD = 0x0004;
constexpr int WRDE_REUSE = 0x0008;
constexpr int WRDE_SHOWERR = 0x0010;
constexpr int WRDE_UNDEF = 0x0020;

enum {
	WRDE_SUCCESS,
	WRDE_NOSPACE,
	WRDE_BADCHAR,
	WRDE_BADVAL,
	WRDE_CMDSUB,
	WRDE_SYNTAX,
	WRDE_NOSYS,
};

int wordexp(const char* s, wordexp_t* p, int flags);
void wordfree(wordexp_t* p);

}
using namespace CG3_WordExp;

#endif
