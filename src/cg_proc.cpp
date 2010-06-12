/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.	If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "Grammar.h"
#include "BinaryGrammar.h"
#include "ApertiumApplicator.h"
#include "MatxinApplicator.h"
#include "GrammarApplicator.h"

#include <getopt.h>
#include <libgen.h>

#include "version.h"

using namespace std;
using CG3::CG3Quit;

void endProgram(char *name);


void endProgram(char *name) {
	fprintf(stdout, "VISL CG-3 Disambiguator version %u.%u.%u.%u\n",
		CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
	cout << basename(name) <<": process a stream with a constraint grammar" << endl;
	cout << "USAGE: " << basename(name) << " [-t] [-s] [-d] [-r rule] grammar_file [input_file [output_file]]" << endl;
	cout << "Options:" << endl;
#if HAVE_GETOPT_LONG
	cout << "	-d, --disambiguation:	 morphological disambiguation" << endl;
	cout << "	-s, --sections=NUM:	 specify number of sections to process" << endl;
	cout << "	-f, --stream-format=NUM: set the format of the I/O stream to NUM," << endl;
	cout << "				   where `0' is VISL format, `1' is Apertium" << endl;
	cout << "				   format and `2' is Apertium format as input," << endl;
	cout << "				   Matxin format as output (default: 1)" << endl;
	cout << "	-r, --rule=NAME:	 run only the named rule" << endl;
	cout << "	-t, --trace:		 print debug output on stderr" << endl;
	cout << "	-w, --wordform-case:	 enforce surface case on lemma/baseform " << endl;
	cout << "				   (to work with -w option of lt-proc)" << endl;
	cout << "	-n, --no-word-forms:	 do not print out the word form of each cohort" << endl;
	cout << "	-v, --version:	 	 version" << endl;
	cout << "	-h, --help:		 show this help" << endl;
#else
	cout << "	-d:	 morphological disambiguation (default behaviour)" << endl;
	cout << "	-s:	 specify number of sections to process" << endl;
	cout << "	-f: 	 set the format of the I/O stream to NUM," << endl;
	cout << "		   where `0' is VISL format and `1' is " << endl;
	cout << "		   Apertium format (default: 1)" << endl;
	cout << "	-r:	 run only the named rule" << endl;
	cout << "	-t:	 print debug output on stderr" << endl;
	cout << "	-w:	 enforce surface case on lemma/baseform " << endl;
	cout << "		   (to work with -w option of lt-proc)" << endl;
	cout << "	-n:	 do not print out the word form of each cohort" << endl;
	cout << "	-v:	 version" << endl;
	cout << "	-h:	 show this help" << endl;
#endif
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int trace = 0;
	int wordform_case = 0;
	int print_word_forms = 1;
	int cmd = 0;
	int sections = 0;
	int stream_format = 1;
	bool nullFlush=false;
	char* single_rule = 0;

	UErrorCode status = U_ZERO_ERROR;
	UFILE *ux_stdin = 0;
	UFILE *ux_stdout = 0;
	UFILE *ux_stderr = 0;

#if HAVE_GETOPT_LONG
	static struct option long_options[] = {
		{"disambiguation",	0, 0, 'd'},
		{"sections", 		0, 0, 's'},
		{"stream-format",	required_argument, 0, 'f'},
		{"rule", 		0, 0, 'r'},
		{"trace", 		0, 0, 't'},
		{"wordform-case",	0, 0, 'w'},
		{"no-word-forms",	0, 0, 'n'},
		{"version",   		0, 0, 'v'},
		{"help",		0, 0, 'h'},
		{"null-flush",		0, 0, 'z'}
	};
#endif

	// This is to make pedantic compilers not complain about the while (true) condition...silly MSVC.
	int c = 0;
	while (c != -1) {
#if HAVE_GETOPT_LONG
		int option_index;
		c = getopt_long(argc, argv, "ds:f:tr:nwvhz", long_options, &option_index);
#else
		c = getopt(argc, argv, "ds:f:tr:inwvhz");
#endif		
		if (c == -1) {
			break;
		}
			
		switch(c) {

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
				trace = 1;
				break;

			case 'r': 
				single_rule = strdup(optarg);
			case 's':
				sections = atoi(optarg);
				break;
				
			case 'n':
				print_word_forms = 0;
				break;

			case 'w':
				wordform_case = 1;
				break;

			case 'v':
				fprintf(stdout, "VISL CG-3 Disambiguator version %u.%u.%u.%u\n",
					CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
	
				exit(EXIT_SUCCESS);
				break;
			case 'z':
				nullFlush=true;
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

	const char *codepage_default = ucnv_getDefaultName();
	ucnv_setDefaultName("UTF-8");
	const char *locale_default = "en_US_POSIX"; //uloc_getDefault();

	ux_stdin = u_finit(stdin, locale_default, codepage_default);
	ux_stdout = u_finit(stdout, locale_default, codepage_default);
	ux_stderr = u_finit(stderr, locale_default, codepage_default);

	CG3::Grammar grammar;

	CG3::IGrammarParser *parser = 0;
	
	if (optind == (argc - 3)) {

		FILE *in = fopen(argv[optind], "rb");
		if (in == NULL || ferror(in)) {
			endProgram(argv[0]);
		}
		
		u_fclose(ux_stdin);
		ux_stdin = u_fopen(argv[optind+1], "rb", locale_default, codepage_default);
		if (ux_stdin == NULL) {
			endProgram(argv[0]);
		}
		
		u_fclose(ux_stdout);
		ux_stdout = u_fopen(argv[optind+2], "wb", locale_default, codepage_default);
		if (ux_stdout == NULL) {
			endProgram(argv[0]);
		}

		fread(&CG3::cbuffers[0][0], 1, 4, in);
		fclose(in);

	}
	else if (optind == (argc -2)) {

		FILE *in = fopen(argv[optind], "rb");
		if (in == NULL || ferror(in)) {
			endProgram(argv[0]);
		}
		
		u_fclose(ux_stdin);
		ux_stdin = u_fopen(argv[optind+1], "rb", locale_default, codepage_default);
		if (ux_stdin == NULL) {
			endProgram(argv[0]);
		}
		
		fread(&CG3::cbuffers[0][0], 1, 4, in);
		fclose(in);
		
	}
	else if (optind == (argc - 1)) {

		FILE *in = fopen(argv[optind], "rb");
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

	if (CG3::cbuffers[0][0] == 'C' && CG3::cbuffers[0][1] == 'G' && CG3::cbuffers[0][2] == '3' && CG3::cbuffers[0][3] == 'B') {
		//std::cerr << "Info: Binary grammar detected." << std::endl;
		parser = new CG3::BinaryGrammar(grammar, ux_stderr);
	}
	else {
		std::cerr << "Info: Text grammar detected -- to process textual " << std::endl;
		std::cerr << "grammars, use `vislcg3', to compile this grammar, use `cg-comp'" << std::endl;

		//parser = new CG3::TextualParser(ux_stderr);
		CG3Quit(1);
	}

	grammar.ux_stderr = ux_stderr;

	if (parser->parse_grammar_from_file(argv[optind], locale_default, codepage_default)) {
		std::cerr << "Error: Grammar could not be parsed - exiting!" << std::endl;
		CG3Quit(1);
	}

	grammar.reindex();

	delete parser;
	parser = 0;

	CG3::GrammarApplicator *applicator = 0;

	if (stream_format == 0) {
		applicator = new CG3::GrammarApplicator(ux_stderr);
	}
	else if (stream_format == 2) {
		CG3::MatxinApplicator* matxinApplicator= new CG3::MatxinApplicator(ux_stderr);
		matxinApplicator->setNullFlush(nullFlush);
		if (wordform_case == 1) {
			matxinApplicator->wordform_case = true;
		}
		if (print_word_forms == 0) {
			matxinApplicator->print_word_forms = false;
		}
		applicator = matxinApplicator;
	}
	else {
		CG3::ApertiumApplicator* apertiumApplicator= new CG3::ApertiumApplicator(ux_stderr);
		apertiumApplicator->setNullFlush(nullFlush);
		if (wordform_case == 1) {
			apertiumApplicator->wordform_case = true;
		}
		if (print_word_forms == 0) {
			apertiumApplicator->print_word_forms = false;
		}
		applicator = apertiumApplicator;
	}

	applicator->setGrammar(&grammar);
	for (int32_t i=1 ; i<=sections ; i++) {
		applicator->sections.push_back(i);
	}

	if (trace == 1) {
		applicator->trace = true;
	}

	// This is if we want to run a single rule  (-r option)
	if (single_rule != 0) {
		size_t sn = strlen(single_rule);
		UChar *buf = new UChar[sn*3];
		buf[0] = 0;
		buf[sn] = 0;
		u_charsToUChars(single_rule, buf, sn);
		const_foreach(CG3::RuleByLineHashMap, applicator->grammar->rule_by_line, riter, riter_end) {
			const CG3::Rule *rule = riter->second;
			if (rule->name && u_strcmp(rule->name, buf) == 0) {
				applicator->valid_rules.push_back(rule->line);
			}
		}
		delete[] buf;
	}

	try {
		switch(cmd) {

			case 'd':
			default:
				applicator->runGrammarOnText(ux_stdin, ux_stdout);
				break;
		}

	}
	catch (exception& e) {
		cerr << e.what();
		exit(1);
	}

	delete applicator;
	applicator = 0;

	u_fclose(ux_stdin);
	u_fclose(ux_stdout);
	u_fclose(ux_stderr);

	u_cleanup();

	return EXIT_SUCCESS;
}
