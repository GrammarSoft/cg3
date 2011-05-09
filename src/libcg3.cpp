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
#include "GrammarWriter.h"
#include "BinaryGrammar.h"
#include "GrammarApplicator.h"
#include "version.h"
using namespace CG3;

typedef Grammar cg3_grammar_t;
typedef GrammarApplicator cg3_applicator_t;

#define _CG3_INTERNAL
#include "cg3.h"

UFILE *ux_stdin = 0;
UFILE *ux_stdout = 0;
UFILE *ux_stderr = 0;

int cg3_init(FILE *in, FILE *out, FILE *err) {
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

int cg3_cleanup(void) {
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
