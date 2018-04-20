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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.	If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.hpp"
#include "Grammar.hpp"
#include "TextualParser.hpp"
#include "BinaryGrammar.hpp"
#include "ApertiumApplicator.hpp"
#include "MatxinApplicator.hpp"
#include "GrammarApplicator.hpp"

#include <getopt.h>
#ifndef _WIN32
#include <libgen.h>
#endif

#include "version.hpp"

using CG3::CG3Quit;

void endProgram(char* name) {
	using namespace std;
	fprintf(stdout, "VISL CG-3 Disambiguator version %u.%u.%u.%u\n",
	  CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
	cout << basename(name) << ": process a stream with a constraint grammar" << endl;
	cout << "USAGE: " << basename(name) << " [-t] [-s] [-d] [-r rule] grammar_file [input_file [output_file]]" << endl;
	cout << "Options:" << endl;
#if HAVE_GETOPT_LONG
	cout << "	-d, --disambiguation:	 morphological disambiguation" << endl;
	cout << "	-s, --sections=NUM:	 specify number of sections to process" << endl;
	cout << "	-f, --stream-format=NUM: set the format of the I/O stream to NUM," << endl;
	cout << "				   where `0' is VISL format, `1' is Apertium" << endl;
	cout << "				   format (default: 1)" << endl;
	cout << "	-r, --rule=NAME:	 run only the named rule" << endl;
	cout << "	-t, --trace:		 print debug output on stderr" << endl;
	cout << "	-w, --wordform-case:	 enforce surface case on lemma/baseform " << endl;
	cout << "				   (to work with -w option of lt-proc)" << endl;
	cout << "	-n, --no-word-forms:	 do not print out the word form of each cohort" << endl;
	cout << "	-1, --first:	 	 only output the first analysis if ambiguity remains" << endl;
	cout << "	-z, --null-flush:	flush output on the null character" << endl;

	cout << "	-v, --version:	 	 version" << endl;
	cout << "	-h, --help:		 show this help" << endl;
#else
	cout << "	-d:	 morphological disambiguation (default behaviour)" << endl;
	cout << "	-s:	 specify number of sections to process" << endl;
	cout << "	-f: 	 set the format of the I/O stream to NUM," << endl;
	cout << "		   where `0' is VISL format, `1' is " << endl;
	cout << "		   Apertium format and `2' is Matxin (default: 1)" << endl;
	cout << "	-r:	 run only the named rule" << endl;
	cout << "	-t:	 print debug output on stderr" << endl;
	cout << "	-w:	 enforce surface case on lemma/baseform " << endl;
	cout << "		   (to work with -w option of lt-proc)" << endl;
	cout << "	-n:	 do not print out the word form of each cohort" << endl;
	cout << "	-1:	 only output the first analysis if ambiguity remains" << endl;
	cout << "	-z:	 flush output on the null character" << endl;

	cout << "	-v:	 version" << endl;
	cout << "	-h:	 show this help" << endl;
#endif
	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
	bool trace = false;
	bool wordform_case = false;
	bool print_word_forms = true;
	bool only_first = false;
	int cmd = 0;
	int sections = 0;
	int stream_format = 1;
	bool null_flush = false;
	std::string single_rule;

	UErrorCode status = U_ZERO_ERROR;

#if HAVE_GETOPT_LONG
	struct option long_options[] = {
		{"disambiguation",	0, 0, 'd'},
		{"sections", 		0, 0, 's'},
		{"stream-format",	required_argument, 0, 'f'},
		{"rule", 		0, 0, 'r'},
		{"trace", 		0, 0, 't'},
		{"wordform-case",	0, 0, 'w'},
		{"no-word-forms",	0, 0, 'n'},
		{"version",   		0, 0, 'v'},
		{"first",   		0, 0, '1'},
		{"help",		0, 0, 'h'},
		{"null-flush",		0, 0, 'z'},
	};
#endif

	// This is to make pedantic compilers not complain about the while (true) condition...silly MSVC.
	int c = 0;
	while (c != -1) {
#if HAVE_GETOPT_LONG
		int option_index;
		c = getopt_long(argc, argv, "ds:f:tr:n1wvhz", long_options, &option_index);
#else
		c = getopt(argc, argv, "ds:f:tr:in1wvhz");
#endif
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'd':
			if (cmd == 0) {
				cmd = c;
			}
			else {
				endProgram(argv[0]);
			}
			break;

		case 'f':
			stream_format = atoi(optarg);
			break;

		case 't':
			trace = true;
			break;

		case 'r': {
			// strdup() is Posix
			size_t len = strlen(optarg) + 1;
			single_rule.reserve(len);
			std::copy(optarg, optarg + len, std::back_inserter(single_rule));
			break;
		}
		case 's':
			sections = atoi(optarg);
			break;

		case 'n':
			print_word_forms = false;
			break;

		case '1':
			only_first = true;
			break;

		case 'w':
			wordform_case = true;
			break;

		case 'v':
			fprintf(stdout, "VISL CG-3 Disambiguator version %u.%u.%u.%u\n",
			  CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);

			exit(EXIT_SUCCESS);
			break;
		case 'z':
			null_flush = true;
			break;
		case 'h':
		default:
			endProgram(argv[0]);
			break;
		}
	}

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}

	ucnv_setDefaultName("UTF-8");
	uloc_setDefault("en_US_POSIX", &status);

	CG3::Grammar grammar;

	/* Add a / in front to enable this test...
	{
		CG3::ApertiumApplicator a(0);
		//grammar.sub_readings_ltr = true;
		a.setGrammar(&grammar);
		a.testPR(ux_stdout);
		return 0;
	}
	//*/

	if (optind <= (argc - 1)) {
		FILE* in = fopen(argv[optind], "rb");
		if (in == NULL || ferror(in)) {
			endProgram(argv[0]);
		}

		if (fread(&CG3::cbuffers[0][0], 1, 4, in) != 4) {
			std::cerr << "Error: Error reading first 4 bytes from grammar!" << std::endl;
			CG3Quit(1);
		}
		fclose(in);
	}
	else {
		endProgram(argv[0]);
	}

	std::istream* ux_stdin = &std::cin;
	std::unique_ptr<std::ifstream> _ux_stdin;
	if (optind <= (argc - 2)) {
		_ux_stdin.reset(new std::ifstream(argv[optind + 1], std::ios::binary));

		if (!_ux_stdin || _ux_stdin->bad()) {
			endProgram(argv[0]);
		}
		ux_stdin = _ux_stdin.get();
	}

	std::ostream* ux_stdout = &std::cout;
	std::unique_ptr<std::ofstream> _ux_stdout;
	if (optind <= (argc - 3)) {
		_ux_stdout.reset(new std::ofstream(argv[optind + 2], std::ios::binary));

		if (!_ux_stdout || _ux_stdout->bad()) {
			endProgram(argv[0]);
		}
		ux_stdout = _ux_stdout.get();
	}

	std::unique_ptr<CG3::IGrammarParser> parser;
	if (CG3::cbuffers[0][0] == 'C' && CG3::cbuffers[0][1] == 'G' && CG3::cbuffers[0][2] == '3' && CG3::cbuffers[0][3] == 'B') {
		parser.reset(new CG3::BinaryGrammar(grammar, std::cerr));
	}
	else {
		// Forbidding text grammars makes debugging very annoying
		std::cerr << "Warning: Text grammar detected - to better process textual" << std::endl;
		std::cerr << "grammars, use `vislcg3'; to compile this grammar, use `cg-comp'" << std::endl;
		parser.reset(new CG3::TextualParser(grammar, std::cerr));
	}

	grammar.ux_stderr = &std::cerr;

	if (parser->parse_grammar(argv[optind])) {
		std::cerr << "Error: Grammar could not be parsed - exiting!" << std::endl;
		CG3Quit(1);
	}

	grammar.reindex();

	std::unique_ptr<CG3::GrammarApplicator> applicator;

	if (stream_format == 0) {
		applicator.reset(new CG3::GrammarApplicator(std::cerr));
	}
	else if (stream_format == 2) {
		CG3::MatxinApplicator* matxinApplicator = new CG3::MatxinApplicator(std::cerr);
		matxinApplicator->setNullFlush(null_flush);
		matxinApplicator->wordform_case = wordform_case;
		matxinApplicator->print_word_forms = print_word_forms;
		matxinApplicator->print_only_first = only_first;
		applicator.reset(matxinApplicator);
	}
	else {
		CG3::ApertiumApplicator* apertiumApplicator = new CG3::ApertiumApplicator(std::cerr);
		apertiumApplicator->setNullFlush(null_flush);
		apertiumApplicator->wordform_case = wordform_case;
		apertiumApplicator->print_word_forms = print_word_forms;
		apertiumApplicator->print_only_first = only_first;
		applicator.reset(apertiumApplicator);
	}

	applicator->setGrammar(&grammar);
	for (int32_t i = 1; i <= sections; i++) {
		applicator->sections.push_back(i);
	}

	applicator->trace = trace;
	applicator->unicode_tags = true;
	applicator->unique_tags = false;

	// This is if we want to run a single rule  (-r option)
	if (!single_rule.empty()) {
		size_t sn = single_rule.size();
		CG3::UString buf(sn * 3, 0);
		u_charsToUChars(single_rule.c_str(), &buf[0], sn);
		for (auto rule : applicator->grammar->rule_by_number) {
			if (rule->name == buf) {
				applicator->valid_rules.push_back(rule->number);
			}
		}
	}

	try {
		switch (cmd) {
		case 'd':
		default:
			applicator->runGrammarOnText(*ux_stdin, *ux_stdout);
			break;
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what();
		exit(1);
	}

	u_cleanup();
}
