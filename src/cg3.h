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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CG3_INTERNAL
	typedef struct _cg3_grammar_t    cg3_grammar_t;
	typedef struct _cg3_applicator_t cg3_applicator_t;
	typedef struct _cg3_sentence_t   cg3_sentence_t;
	typedef struct _cg3_cohort_t     cg3_cohort_t;
	typedef struct _cg3_reading_t    cg3_reading_t;
	typedef struct _cg3_tag_t        cg3_tag_t;
#endif

enum cg3_status_t {
	CG3_ERROR   = 0,
	CG3_SUCCESS = 1,
};

// Default usage: if (!cg3_init(stdin, stdout, stderr)) { exit(1); }
cg3_status_t cg3_init(FILE *in, FILE *out, FILE *err);
// Default usage: cg3_cleanup();
cg3_status_t cg3_cleanup(void);

cg3_grammar_t *cg3_grammar_load(const char *filename);
void cg3_grammar_free(cg3_grammar_t *grammar);

cg3_applicator_t *cg3_applicator_create(cg3_grammar_t *grammar);
void cg3_applicator_free(cg3_applicator_t *applicator);

cg3_sentence_t *cg3_sentence_new(cg3_applicator_t *applicator);
void cg3_sentence_runrules(cg3_applicator_t *applicator, cg3_sentence_t *sentence);
// The Sentence takes ownership of the Cohort here.
void cg3_sentence_addcohort(cg3_sentence_t *sentence, cg3_cohort_t *cohort);
void cg3_sentence_free(cg3_sentence_t *sentence);

cg3_cohort_t *cg3_cohort_create(cg3_sentence_t *sentence);
void cg3_cohort_setwordform(cg3_cohort_t *cohort, cg3_tag_t *wordform);
void cg3_cohort_setdependency(cg3_cohort_t *cohort, uint32_t dep_self, uint32_t dep_parent);
// The Cohort takes ownership of the Reading here.
void cg3_cohort_addreading(cg3_cohort_t *cohort, cg3_reading_t *reading);
// This is usually not to be used. The Sentence will take ownership of the Cohort and free it on destruction
void cg3_cohort_free(cg3_cohort_t *cohort);

cg3_reading_t *cg3_reading_create(cg3_cohort_t *cohort);
cg3_status_t cg3_reading_addtag(cg3_reading_t *reading, cg3_tag_t *tag);
// This is usually not to be used. The Cohort will take ownership of the Reading and free it on destruction
void cg3_reading_free(cg3_reading_t *reading);

#ifdef U_ICU_VERSION_MAJOR_NUM
cg3_tag_t *cg3_tag_create_u(cg3_applicator_t *applicator, const UChar *text);
#endif
cg3_tag_t *cg3_tag_create_u8(cg3_applicator_t *applicator, const char *text);
cg3_tag_t *cg3_tag_create_u16(cg3_applicator_t *applicator, const uint16_t *text);
cg3_tag_t *cg3_tag_create_u32(cg3_applicator_t *applicator, const uint32_t *text);
cg3_tag_t *cg3_tag_create_w(cg3_applicator_t *applicator, const wchar_t *text);

#ifdef __cplusplus
}
#endif

#endif
