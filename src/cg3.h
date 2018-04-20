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
#ifndef c6d28b7452ec699b_CG3_H
#define c6d28b7452ec699b_CG3_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ < 199901L)
	#error "CG-3 requires C99 or newer!"
#endif

typedef void cg3_grammar;
typedef void cg3_applicator;
typedef cg3_applicator cg3_mwesplitapplicator;
typedef void cg3_sentence;
typedef void cg3_cohort;
typedef void cg3_reading;
typedef void cg3_tag;
typedef void std_istream;
typedef void std_ostream;

typedef enum {
	CG3_ERROR   = 0,
	CG3_SUCCESS = 1,
} cg3_status;

typedef enum {
	CG3F_ORDERED            = (1 <<  0),
	CG3F_UNSAFE             = (1 <<  1),
	CG3F_NO_MAPPINGS        = (1 <<  2),
	CG3F_NO_CORRECTIONS     = (1 <<  3),
	CG3F_NO_BEFORE_SECTIONS = (1 <<  4),
	CG3F_NO_SECTIONS        = (1 <<  5),
	CG3F_NO_AFTER_SECTIONS  = (1 <<  6),
	CG3F_TRACE              = (1 <<  7),
	CG3F_SINGLE_RUN         = (1 <<  8),
	CG3F_ALWAYS_SPAN        = (1 <<  9),
	CG3F_DEP_ALLOW_LOOPS    = (1 << 10),
	CG3F_DEP_NO_CROSSING    = (1 << 11),
	CG3F_NO_PASS_ORIGIN     = (1 << 13),
} cg3_flags;

typedef enum {
	CG3O_SECTIONS      = 1,
	CG3O_SECTIONS_TEXT = 2,
} cg3_option;

// Default usage: if (!cg3_init(stdin, stdout, stderr)) { exit(1); }
cg3_status cg3_init(FILE* in, FILE* out, FILE* err);
// Default usage: cg3_cleanup();
cg3_status cg3_cleanup(void);

cg3_grammar* cg3_grammar_load(const char* filename);
cg3_grammar* cg3_grammar_load_buffer(const char* buffer, size_t length);
void cg3_grammar_free(cg3_grammar* grammar);

cg3_applicator* cg3_applicator_create(cg3_grammar* grammar);
// Pass in OR'ed values from cg3_flags; each call resets flags, so set all needed ones in a single call.
void cg3_applicator_setflags(cg3_applicator* applicator, uint32_t flags);
/*
Valid signatures:
- cg3_applicator_setoption(aplc, CG3O_SECTIONS, uint32_t*);
	Pointer to an uint32_t stating the max section to run; akin to --sections 6
- cg3_applicator_setoption(aplc, CG3O_SECTIONS_TEXT, const char*);
	Pointer to cstring with ranges; akin to --sections "2,4-6,8-9"
//*/
void cg3_applicator_setoption(cg3_applicator* applicator, cg3_option option, void* value);
void cg3_applicator_free(cg3_applicator* applicator);

void cg3_run_grammar_on_text(cg3_applicator*, std_istream*, std_ostream*);

cg3_mwesplitapplicator* cg3_mwesplitapplicator_create();
#define cg3_mwesplitapplicator_free cg3_applicator_free
#define cg3_run_mwesplit_on_text cg3_run_grammar_on_text

cg3_sentence* cg3_sentence_new(cg3_applicator* applicator);
cg3_sentence* cg3_sentence_copy(cg3_sentence* from, cg3_applicator* to);
void cg3_sentence_runrules(cg3_applicator* applicator, cg3_sentence* sentence);
// The Sentence takes ownership of the Cohort here.
void cg3_sentence_addcohort(cg3_sentence* sentence, cg3_cohort* cohort);
size_t cg3_sentence_numcohorts(cg3_sentence* sentence);
cg3_cohort* cg3_sentence_getcohort(cg3_sentence* sentence, size_t which);
void cg3_sentence_free(cg3_sentence* sentence);

cg3_cohort* cg3_cohort_create(cg3_sentence* sentence);
void cg3_cohort_setwordform(cg3_cohort* cohort, cg3_tag* wordform);
cg3_tag* cg3_cohort_getwordform(cg3_cohort* cohort);
uint32_t cg3_cohort_getid(cg3_cohort* cohort);
void cg3_cohort_setdependency(cg3_cohort* cohort, uint32_t dep_self, uint32_t dep_parent);
void cg3_cohort_getdependency(cg3_cohort* cohort, uint32_t* dep_self, uint32_t* dep_parent);
// The Cohort takes ownership of the Reading here.
void cg3_cohort_addreading(cg3_cohort* cohort, cg3_reading* reading);
size_t cg3_cohort_numreadings(cg3_cohort* cohort);
cg3_reading* cg3_cohort_getreading(cg3_cohort* cohort, size_t which);
// This is usually not to be used. The Sentence will take ownership of the Cohort and free it on destruction
void cg3_cohort_free(cg3_cohort* cohort);

cg3_reading* cg3_reading_create(cg3_cohort* cohort);
cg3_status cg3_reading_addtag(cg3_reading* reading, cg3_tag* tag);
size_t cg3_reading_numtags(cg3_reading* reading);
cg3_tag* cg3_reading_gettag(cg3_reading* reading, size_t which);
size_t cg3_reading_numtraces(cg3_reading* reading);
uint32_t cg3_reading_gettrace(cg3_reading* reading, size_t which);
// This is usually not to be used. The Cohort will take ownership of the Reading and free it on destruction
void cg3_reading_free(cg3_reading* reading);

cg3_reading* cg3_subreading_create(cg3_reading* reading);
// The Reading takes ownership of the Sub-Reading here.
cg3_status cg3_reading_setsubreading(cg3_reading* reading, cg3_reading* subreading);
size_t cg3_reading_numsubreadings(cg3_reading* reading);
cg3_reading* cg3_reading_getsubreading(cg3_reading* reading, size_t which);
// This is usually not to be used. The Reading will take ownership of the Sub-Reading and free it on destruction
void cg3_subreading_free(cg3_reading* subreading);

#ifdef U_ICU_VERSION_MAJOR_NUM
cg3_tag *cg3_tag_create_u(cg3_applicator *applicator, const UChar *text);
#endif
cg3_tag* cg3_tag_create_u8(cg3_applicator* applicator, const char* text);
cg3_tag* cg3_tag_create_u16(cg3_applicator* applicator, const uint16_t* text);
cg3_tag* cg3_tag_create_u32(cg3_applicator* applicator, const uint32_t* text);
cg3_tag* cg3_tag_create_w(cg3_applicator* applicator, const wchar_t* text);

#ifdef U_ICU_VERSION_MAJOR_NUM
const UChar *cg3_tag_gettext_u(cg3_tag *tag);
#endif
const char* cg3_tag_gettext_u8(cg3_tag* tag);
const uint16_t* cg3_tag_gettext_u16(cg3_tag* tag);
const uint32_t* cg3_tag_gettext_u32(cg3_tag* tag);
const wchar_t* cg3_tag_gettext_w(cg3_tag* tag);

// These 3 from Paul Meurer <paul.meurer@uni.no>
size_t cg3_cohort_numdelreadings(cg3_cohort* cohort);
cg3_reading* cg3_cohort_getdelreading(cg3_cohort* cohort, size_t which);
size_t cg3_reading_gettrace_ruletype(cg3_reading* reading_, size_t which);

#ifdef __cplusplus
}
#endif

#endif
