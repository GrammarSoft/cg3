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
#include "GrammarWriter.hpp"
#include "BinaryGrammar.hpp"
#include "TextualParser.hpp"
#include "Relabeller.hpp"

#ifndef _WIN32
#include <libgen.h>
#endif

#include "version.hpp"

using CG3::CG3Quit;

void endProgram(char* name) {
	if (name != NULL) {
		fprintf(stdout, "VISL CG-3 Relabeller version %u.%u.%u.%u\n",
		  CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
		std::cout << basename(name) << ": relabel a binary grammar using a relabelling file" << std::endl;
		std::cout << "USAGE: " << basename(name) << " input_grammar_file relabel_rule_file output_grammar_file" << std::endl;
	}
	exit(EXIT_FAILURE);
}

// like libcg3's, but with a non-void grammar …
CG3::Grammar* cg3_grammar_load(const char* filename, std::ostream& ux_stdout, std::ostream& ux_stderr, bool require_binary = false) {
	using namespace CG3;
	std::ifstream input(filename, std::ios::binary);
	if (!input) {
		u_fprintf(ux_stderr, "Error: Error opening %s for reading!\n", filename);
		return 0;
	}
	if (!input.read(&cbuffers[0][0], 4)) {
		u_fprintf(ux_stderr, "Error: Error reading first 4 bytes from grammar!\n");
		return 0;
	}
	input.close();

	Grammar* grammar = new Grammar;
	grammar->ux_stderr = &ux_stderr;
	grammar->ux_stdout = &ux_stdout;

	std::unique_ptr<IGrammarParser> parser;

	if (cbuffers[0][0] == 'C' && cbuffers[0][1] == 'G' && cbuffers[0][2] == '3' && cbuffers[0][3] == 'B') {
		parser.reset(new BinaryGrammar(*grammar, ux_stderr));
	}
	else {
		if (require_binary) {
			u_fprintf(ux_stderr, "Error: Text grammar detected -- to compile this grammar, use `cg-comp'\n");
			CG3Quit(1);
		}
		parser.reset(new TextualParser(*grammar, ux_stderr));
	}
	if (parser->parse_grammar(filename)) {
		u_fprintf(ux_stderr, "Error: Grammar could not be parsed!\n");
		return 0;
	}

	grammar->reindex();

	return grammar;
}

int main(int argc, char* argv[]) {
	UErrorCode status = U_ZERO_ERROR;

	if (argc != 4) {
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

	std::unique_ptr<CG3::Grammar> grammar{ cg3_grammar_load(argv[1], std::cout, std::cerr, true) };
	std::unique_ptr<CG3::Grammar> relabel_grammar{ cg3_grammar_load(argv[2], std::cout, std::cerr) };

	CG3::Relabeller relabeller(*grammar, *relabel_grammar, std::cerr);
	relabeller.relabel();

	FILE* gout = fopen(argv[3], "wb");
	if (gout) {
		CG3::BinaryGrammar writer(*grammar, std::cerr);
		writer.writeBinaryGrammar(gout);
	}
	else {
		std::cerr << "Could not write grammar to " << argv[3] << std::endl;
	}

	u_cleanup();

	return status;
}
