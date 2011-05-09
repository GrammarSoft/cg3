/*
* Copyright (C) 2007-2011, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_CG3_H
#define c6d28b7452ec699b_CG3_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CG3_INTERNAL
	typedef struct _cg3_grammar_t cg3_grammar_t;
	typedef struct _cg3_applicator_t cg3_applicator_t;
#endif

enum {
	CG3_ERROR = 0,
	CG3_SUCCESS = 1,
};

// Default usage: if (!cg3_init(stdin, stdout, stderr)) { exit(1); }
int cg3_init(FILE *in, FILE *out, FILE *err);
// Default usage: cg3_cleanup();
int cg3_cleanup(void);

cg3_grammar_t *cg3_grammar_load(const char *filename);
void cg3_grammar_free(cg3_grammar_t *grammar);

cg3_applicator_t *cg3_applicator_create(cg3_grammar_t *grammar);
void cg3_applicator_free(cg3_applicator_t *applicator);

#ifdef __cplusplus
}
#endif

#endif
