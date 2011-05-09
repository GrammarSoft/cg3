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

#include "stdafx.h"
#include "icu_uoptions.h"
#include "Grammar.h"
#include "TextualParser.h"
#include "BinaryGrammar.h"
#include "GrammarApplicator.h"
#include "Window.h"
#include "SingleWindow.h"
#include "version.h"
using namespace CG3;

typedef Grammar           cg3_grammar_t;
typedef GrammarApplicator cg3_applicator_t;
typedef SingleWindow      cg3_sentence_t;
typedef Cohort            cg3_cohort_t;
typedef Reading           cg3_reading_t;
typedef Tag               cg3_tag_t;

#define _CG3_INTERNAL
#include "cg3.h"

namespace {
	UFILE *ux_stdin = 0;
	UFILE *ux_stdout = 0;
	UFILE *ux_stderr = 0;
}

cg3_status_t cg3_init(FILE *in, FILE *out, FILE *err) {
	UErrorCode status = U_ZERO_ERROR;
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		fprintf(err, "CG3 Error: Cannot initialize ICU. Status = %s\n", u_errorName(status));
		return CG3_ERROR;
	}
	status = U_ZERO_ERROR;

	ucnv_setDefaultName("UTF-8");

	uloc_setDefault("en_US_POSIX", &status);
	if (U_FAILURE(status)) {
		fprintf(err, "CG3 Error: Failed to set default locale. Status = %s\n", u_errorName(status));
		return CG3_ERROR;
	}
	status = U_ZERO_ERROR;

	ux_stdin = u_finit(in, uloc_getDefault(), ucnv_getDefaultName());
	if (!ux_stdin) {
		fprintf(err, "CG3 Error: The input stream could not be inited.\n");
		return CG3_ERROR;
	}

	ux_stdout = u_finit(out, uloc_getDefault(), ucnv_getDefaultName());
	if (!ux_stdout) {
		fprintf(err, "CG3 Error: The output stream could not be inited.\n");
		return CG3_ERROR;
	}

	ux_stderr = u_finit(err, uloc_getDefault(), ucnv_getDefaultName());
	if (!ux_stderr) {
		fprintf(err, "CG3 Error: The error stream could not be inited.\n");
		return CG3_ERROR;
	}

	return CG3_SUCCESS;
}

cg3_status_t cg3_cleanup(void) {
	u_fclose(ux_stdin);
	u_fclose(ux_stdout);
	u_fclose(ux_stderr);

	u_cleanup();

	return CG3_SUCCESS;
}

cg3_grammar_t *cg3_grammar_load(const char *filename) {
	std::ifstream input(filename, std::ios::binary);
	if (!input) {
		u_fprintf(ux_stderr, "CG3 Error: Error opening %s for reading!\n", filename);
		return 0;
	}
	if (!input.read(&cbuffers[0][0], 4)) {
		u_fprintf(ux_stderr, "CG3 Error: Error reading first 4 bytes from grammar!\n");
		return 0;
	}
	input.close();

	Grammar *grammar = new Grammar;
	grammar->ux_stderr = ux_stderr;
	grammar->ux_stdout = ux_stdout;

	std::auto_ptr<IGrammarParser> parser;

	if (cbuffers[0][0] == 'C' && cbuffers[0][1] == 'G' && cbuffers[0][2] == '3' && cbuffers[0][3] == 'B') {
		u_fprintf(ux_stderr, "CG3 Info: Binary grammar detected.\n");
		parser.reset(new BinaryGrammar(*grammar, ux_stderr));
	}
	else {
		parser.reset(new TextualParser(*grammar, ux_stderr));
	}
	if (parser->parse_grammar_from_file(filename, uloc_getDefault(), ucnv_getDefaultName())) {
		u_fprintf(ux_stderr, "CG3 Error: Grammar could not be parsed!\n");
		return 0;
	}

	grammar->reindex();

	return grammar;
}

void cg3_grammar_free(cg3_grammar_t *grammar) {
	delete grammar;
}

cg3_applicator_t *cg3_applicator_create(cg3_grammar_t *grammar) {
	GrammarApplicator *applicator = new GrammarApplicator(ux_stderr);
	applicator->setGrammar(grammar);
	applicator->index();
	return applicator;
}

void cg3_applicator_free(cg3_applicator_t *applicator) {
	delete applicator;
}

cg3_sentence_t *cg3_sentence_new(cg3_applicator_t *applicator) {
	SingleWindow *current = applicator->gWindow->allocSingleWindow();
	applicator->initEmptySingleWindow(current);
	return current;
}

void cg3_sentence_runrules(cg3_applicator_t *applicator, cg3_sentence_t *sentence) {
	applicator->gWindow->current = sentence;
	applicator->runGrammarOnWindow();
	applicator->gWindow->current = 0;
}

size_t cg3_sentence_numcohorts(cg3_sentence_t *sentence) {
	return sentence->cohorts.size();
}

cg3_cohort_t *cg3_sentence_getcohort(cg3_sentence_t *sentence, size_t which) {
	return sentence->cohorts[which];
}

void cg3_sentence_free(cg3_sentence_t *sentence) {
	delete sentence;
}

void cg3_sentence_addcohort(cg3_sentence_t *sentence, cg3_cohort_t *cohort) {
	sentence->appendCohort(cohort);
}

cg3_cohort_t *cg3_cohort_create(cg3_sentence_t *sentence) {
	Cohort *cohort = new Cohort(sentence);
	cohort->global_number = sentence->parent->cohort_counter++;
	return cohort;
}

void cg3_cohort_setwordform(cg3_cohort_t *cohort, cg3_tag_t *wordform) {
	cohort->wordform = wordform->hash;
}

void cg3_cohort_setdependency(cg3_cohort_t *cohort, uint32_t dep_self, uint32_t dep_parent) {
	cohort->parent->parent->parent->has_dep = true;
	cohort->dep_self = dep_self;
	cohort->dep_parent = dep_parent;
}

void cg3_cohort_addreading(cg3_cohort_t *cohort, cg3_reading_t *reading) {
	cohort->appendReading(reading);
}

size_t cg3_cohort_numreadings(cg3_cohort_t *cohort) {
	return cohort->readings.size();
}

cg3_reading_t *cg3_cohort_getreading(cg3_cohort_t *cohort, size_t which) {
	ReadingList::iterator it = cohort->readings.begin();
	std::advance(it, which);
	return *it;
}

void cg3_cohort_free(cg3_cohort_t *cohort) {
	delete cohort;
}

cg3_reading_t *cg3_reading_create(cg3_cohort_t *cohort) {
	GrammarApplicator *ga = cohort->parent->parent->parent;
	Reading *reading = new Reading(cohort);
	reading->wordform = cohort->wordform;
	insert_if_exists(reading->parent->possible_sets, ga->grammar->sets_any);
	ga->addTagToReading(*reading, reading->wordform);
	return reading;
}

cg3_status_t cg3_reading_addtag(cg3_reading_t *reading, cg3_tag_t *tag) {
	if (tag->type & T_MAPPING) {
		if (reading->mapping && reading->mapping != tag) {
			u_fprintf(ux_stderr, "CG3 Error: Cannot add a mapping tag to a reading which already is mapped!\n");
			return CG3_ERROR;
		}
	}

	GrammarApplicator *ga = reading->parent->parent->parent->parent;
	ga->addTagToReading(*reading, tag->hash);

	return CG3_SUCCESS;
}

size_t cg3_reading_numtags(cg3_reading_t *reading) {
	return reading->tags_list.size();
}

cg3_tag_t *cg3_reading_gettag(cg3_reading_t *reading, size_t which) {
	uint32List::iterator it = reading->tags_list.begin();
	std::advance(it, which);
	GrammarApplicator *ga = reading->parent->parent->parent->parent;
	return ga->single_tags.find(*it)->second;
}

void cg3_reading_free(cg3_reading_t *reading) {
	delete reading;
}

cg3_tag_t *cg3_tag_create_u(cg3_applicator_t *applicator, const UChar *text) {
	return applicator->addTag(text);
}

cg3_tag_t *cg3_tag_create_u8(cg3_applicator_t *applicator, const char *text) {
	UErrorCode status = U_ZERO_ERROR;

	u_strFromUTF8(&gbuffers[0][0], CG3_BUFFER_SIZE-1, 0, text, strlen(text), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UTF-8 to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

cg3_tag_t *cg3_tag_create_u16(cg3_applicator_t *applicator, const uint16_t *text) {
	return cg3_tag_create_u(applicator, reinterpret_cast<const UChar*>(text));
}

cg3_tag_t *cg3_tag_create_u32(cg3_applicator_t *applicator, const uint32_t *text) {
	UErrorCode status = U_ZERO_ERROR;

	size_t length = 0;
	while (text[length]) {
		++length;
	}

	u_strFromUTF32(&gbuffers[0][0], CG3_BUFFER_SIZE-1, 0, reinterpret_cast<const UChar32*>(text), length, &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UTF-32 to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

cg3_tag_t *cg3_tag_create_w(cg3_applicator_t *applicator, const wchar_t *text) {
	UErrorCode status = U_ZERO_ERROR;

	u_strFromWCS(&gbuffers[0][0], CG3_BUFFER_SIZE-1, 0, text, wcslen(text), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from wchar_t to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

const UChar *cg3_tag_gettext_u(cg3_tag_t *tag) {
	return tag->tag.c_str();
}

const char *cg3_tag_gettext_u8(cg3_tag_t *tag) {
	UErrorCode status = U_ZERO_ERROR;

	u_strToUTF8(&cbuffers[0][0], CG3_BUFFER_SIZE-1, 0, tag->tag.c_str(), tag->tag.length(), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-8. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return &cbuffers[0][0];
}

const uint16_t *cg3_tag_gettext_u16(cg3_tag_t *tag) {
	return reinterpret_cast<const uint16_t*>(tag->tag.c_str());
}

const uint32_t *cg3_tag_gettext_u32(cg3_tag_t *tag) {
	UErrorCode status = U_ZERO_ERROR;

	UChar32 *tmp = reinterpret_cast<UChar32*>(&cbuffers[0][0]);

	u_strToUTF32(tmp, (CG3_BUFFER_SIZE/sizeof(UChar32))-1, 0, tag->tag.c_str(), tag->tag.length(), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-32. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return reinterpret_cast<const uint32_t*>(tmp);
}

const wchar_t *cg3_tag_gettext_w(cg3_tag_t *tag) {
	UErrorCode status = U_ZERO_ERROR;

	wchar_t *tmp = reinterpret_cast<wchar_t*>(&cbuffers[0][0]);

	u_strToWCS(tmp, (CG3_BUFFER_SIZE/sizeof(wchar_t))-1, 0, tag->tag.c_str(), tag->tag.length(), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-32. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return reinterpret_cast<const wchar_t*>(tmp);
}
