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

#include "stdafx.hpp"
#include "Grammar.hpp"
#include "TextualParser.hpp"
#include "GrammarWriter.hpp"
#include "BinaryGrammar.hpp"
#include "GrammarApplicator.hpp"

#ifndef _WIN32
#include <libgen.h>
#endif

#include "version.hpp"

using CG3::CG3Quit;

void endProgram(char* name) {
	if (name != NULL) {
		fprintf(stdout, "VISL CG-3 Compiler version %u.%u.%u.%u\n",
		  CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
		std::cout << basename(name) << ": compile a binary grammar from a text file" << std::endl;
		std::cout << "USAGE: " << basename(name) << " grammar_file output_file" << std::endl;
	}
	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
	UErrorCode status = U_ZERO_ERROR;

	if (argc != 3) {
		endProgram(argv[0]);
	}

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}
	status = U_ZERO_ERROR;

	ucnv_setDefaultName("UTF-8");
	uloc_setDefault("en_US_POSIX", &status);

	CG3::Grammar grammar;

	std::unique_ptr<CG3::IGrammarParser> parser;
	FILE* input = fopen(argv[1], "rb");

	if (!input) {
		std::cerr << "Error: Error opening " << argv[1] << " for reading!" << std::endl;
		CG3Quit(1);
	}
	if (fread(&CG3::cbuffers[0][0], 1, 4, input) != 4) {
		std::cerr << "Error: Error reading first 4 bytes from grammar!" << std::endl;
		CG3Quit(1);
	}
	fclose(input);

	if (CG3::cbuffers[0][0] == 'C' && CG3::cbuffers[0][1] == 'G' && CG3::cbuffers[0][2] == '3' && CG3::cbuffers[0][3] == 'B') {
		std::cerr << "Binary grammar detected. Cannot re-compile binary grammars." << std::endl;
		CG3Quit(1);
	}
	else {
		parser.reset(new CG3::TextualParser(grammar, std::cerr));
	}

	grammar.ux_stderr = &std::cerr;

	if (parser->parse_grammar(argv[1])) {
		std::cerr << "Error: Grammar could not be parsed - exiting!" << std::endl;
		CG3Quit(1);
	}

	grammar.reindex();

	std::cerr << "Sections: " << grammar.sections.size() << ", Rules: " << grammar.rule_by_number.size();
	std::cerr << ", Sets: " << grammar.sets_list.size() << ", Tags: " << grammar.single_tags.size() << std::endl;

	if (grammar.rules_any) {
		std::cerr << grammar.rules_any->size() << " rules cannot be skipped by index." << std::endl;
	}

	if (grammar.has_dep) {
		std::cerr << "Grammar has dependency rules." << std::endl;
	}

	FILE* gout = fopen(argv[2], "wb");

	if (gout) {
		CG3::BinaryGrammar writer(grammar, std::cerr);
		writer.writeBinaryGrammar(gout);
	}
	else {
		std::cerr << "Could not write grammar to " << argv[2] << std::endl;
	}

	u_cleanup();

	return status;
}
