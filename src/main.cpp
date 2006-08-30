/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */

#include "stdafx.h"
#include "icu_uoptions.h"
#include "GrammarParser.h"
#include "Grammar.h"

namespace Options {
	enum OPTIONS {
		HELP1,
		HELP2,
		VERSION,
		GRAMMAR,
		UNSAFE,
		SECTIONS,
		DEBUG,
		STDIN,
		STDOUT,
		STDERR,
		CODEPAGE_ALL,
		CODEPAGE_GRAMMAR,
		CODEPAGE_INPUT,
		CODEPAGE_OUTPUT,
		LOCALE_ALL,
		LOCALE_GRAMMAR,
		LOCALE_INPUT,
		LOCALE_OUTPUT,
		NUM_OPTIONS
	};

	UOption options[]= {
		UOPTION_DEF("help",					'h', UOPT_NO_ARG),
		UOPTION_DEF("?",					'?', UOPT_NO_ARG),
		UOPTION_DEF("version",				'V', UOPT_NO_ARG),
		UOPTION_DEF("grammar",				'g', UOPT_REQUIRES_ARG),
		UOPTION_DEF("unsafe",				'u', UOPT_NO_ARG),
		UOPTION_DEF("sections",				's', UOPT_REQUIRES_ARG),
		UOPTION_DEF("debug",				'd', UOPT_OPTIONAL_ARG),

		UOPTION_DEF("stdin",				'I', UOPT_REQUIRES_ARG),
		UOPTION_DEF("stdout",				'O', UOPT_REQUIRES_ARG),
		UOPTION_DEF("stderr",				'E', UOPT_REQUIRES_ARG),

		UOPTION_DEF("codepage-all",			'C', UOPT_REQUIRES_ARG),
		UOPTION_DEF("codepage-grammar",		0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("codepage-input",		0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("codepage-output",		0, UOPT_REQUIRES_ARG),

		UOPTION_DEF("locale-all",			'L', UOPT_REQUIRES_ARG),
		UOPTION_DEF("locale-grammar",		0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("locale-input",			0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("locale-output",		0, UOPT_REQUIRES_ARG)
	};
}

using namespace Options;

UFILE *ux_stdin = 0;
UFILE *ux_stdout = 0;
UFILE *ux_stderr = 0;

int main(int argc, char* argv[]) {
    UErrorCode status = U_ZERO_ERROR;
	srand((uint32_t)time(0));

    U_MAIN_INIT_ARGS(argc, argv);

	argc = u_parseArgs(argc, argv, (int32_t)(sizeof(options)/sizeof(options[0])), options);

    if (argc < 0) {
        fprintf(stderr, "%s: error in command line argument \"%s\"\n", argv[0], argv[-argc]);
    }

    if (options[VERSION].doesOccur) {
        fprintf(stderr,
                "VISL CG-3 Disambiguator version %s.\n"
                "%s\n",
                CG3_VERSION_STRING, CG3_COPYRIGHT_STRING);
        return U_ZERO_ERROR;
    }

	if (!options[GRAMMAR].doesOccur && !options[HELP1].doesOccur && !options[HELP2].doesOccur) {
		fprintf(stderr, "Error: No grammar specified - cannot continue!\n");
		argc = -argc;
	}

    if (argc < 0 || options[HELP1].doesOccur || options[HELP2].doesOccur) {
        fprintf(stderr, "Usage: vislcg3 [OPTIONS] [FILES]\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, " -h or -? or --help       Displays this list.\n");
        fprintf(stderr, " -V or --version          Prints version number.\n");
        fprintf(stderr, " -g or --grammar          Specifies the grammar file to use for disambiguation.\n");
        fprintf(stderr, "\n");
		fprintf(stderr, " -O or --stdout           A file to print out to instead of stdout.\n");
		fprintf(stderr, " -I or --stdin            A file to read input from instead of stdin.\n");
		fprintf(stderr, " -E or --stderr           A file to print errors to instead of stderr.\n");
        fprintf(stderr, "\n");
		fprintf(stderr, " -C or --codepage-all     The codepage to use for grammar, input, and output streams. Defaults to ISO-8859-1.\n");
        fprintf(stderr, " --codepage-grammar       Codepage to use for grammar. Overwrites --codepage-all.\n");
        fprintf(stderr, " --codepage-input         Codepage to use for input. Overwrites --codepage-all.\n");
        fprintf(stderr, " --codepage-output        Codepage to use for output. Overwrites --codepage-all.\n");
        fprintf(stderr, "\n");
		fprintf(stderr, " -L or --locale-all       The locale to use for grammar, input, and output streams. Defaults to en_US_POSIX.\n");
        fprintf(stderr, " --locale-grammar         Locale to use for grammar. Overwrites --locale-all.\n");
        fprintf(stderr, " --locale-input           Locale to use for input. Overwrites --locale-all.\n");
        fprintf(stderr, " --locale-output          Locale to use for output. Overwrites --locale-all.\n");
        
        return argc < 0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    /* Initialize ICU */
    u_init(&status);
    if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
        fprintf(stderr, "Error: can not initialize ICU.  status = %s\n",
            u_errorName(status));
        return -1;
    }
    status = U_ZERO_ERROR;

	CG3::Grammar *grammar = new CG3::Grammar;
	const char *codepage_grammar = "ISO-8859-1";
	const char *codepage_input   = codepage_grammar;
	const char *codepage_output  = codepage_grammar;

	if (options[CODEPAGE_GRAMMAR].doesOccur) {
		codepage_grammar = options[CODEPAGE_GRAMMAR].value;
	} else if (options[CODEPAGE_ALL].doesOccur) {
		codepage_grammar = options[CODEPAGE_ALL].value;
	}

	if (options[CODEPAGE_INPUT].doesOccur) {
		codepage_input = options[CODEPAGE_INPUT].value;
	} else if (options[CODEPAGE_ALL].doesOccur) {
		codepage_input = options[CODEPAGE_ALL].value;
	}

	if (options[CODEPAGE_OUTPUT].doesOccur) {
		codepage_output = options[CODEPAGE_OUTPUT].value;
	} else if (options[CODEPAGE_ALL].doesOccur) {
		codepage_output = options[CODEPAGE_ALL].value;
	}

	const char *locale_grammar = "en_US_POSIX";
	const char *locale_input   = locale_grammar;
	const char *locale_output  = locale_grammar;

	if (options[LOCALE_GRAMMAR].doesOccur) {
		locale_grammar = options[LOCALE_GRAMMAR].value;
	} else if (options[LOCALE_ALL].doesOccur) {
		locale_grammar = options[LOCALE_ALL].value;
	}

	if (options[LOCALE_INPUT].doesOccur) {
		locale_input = options[LOCALE_INPUT].value;
	} else if (options[LOCALE_ALL].doesOccur) {
		locale_input = options[LOCALE_ALL].value;
	}

	if (options[LOCALE_OUTPUT].doesOccur) {
		locale_output = options[LOCALE_OUTPUT].value;
	} else if (options[LOCALE_ALL].doesOccur) {
		locale_output = options[LOCALE_ALL].value;
	}

	if (!options[STDIN].doesOccur) {
		ux_stdin = u_finit(stdin, locale_input, codepage_input);
	} else {
		ux_stdin = u_fopen(options[STDIN].value, "r", locale_input, codepage_input);
	}

	if (!options[STDOUT].doesOccur) {
		ux_stdout = u_finit(stdout, locale_output, codepage_input);
	} else {
		ux_stdout = u_fopen(options[STDOUT].value, "w", locale_output, codepage_output);
	}

	if (!options[STDERR].doesOccur) {
		ux_stderr = u_finit(stderr, locale_output, codepage_input);
	} else {
		ux_stderr = u_fopen(options[STDERR].value, "w", locale_output, codepage_output);
	}

	if (CG3::GrammarParser::parse_grammar_from_file(options[GRAMMAR].value, locale_grammar, codepage_grammar, grammar)) {
		u_fprintf(ux_stderr, "Error: Grammar could not be parsed - exiting!\n");
		return -1;
	}

	u_fprintf(ux_stdout, "# DELIMITERS does not exist. Instead, look for the set _S_DELIMITERS_\n");

	u_fprintf(ux_stdout, "PREFERRED-TARGETS = ");
	std::vector<UChar*>::iterator iter;
	for(iter = grammar->preferred_targets.begin() ; iter != grammar->preferred_targets.end() ; iter++ ) {
		u_fprintf(ux_stdout, "%S ", *iter);
	}
	u_fprintf(ux_stdout, "\n");

	u_fprintf(ux_stdout, "\n");

	stdext::hash_map<uint32_t, CG3::Set*>::iterator set_iter;
	for (set_iter = grammar->sets.begin() ; set_iter != grammar->sets.end() ; set_iter++) {
		stdext::hash_map<uint32_t, CG3::CompositeTag*>::iterator comp_iter;
		CG3::Set *curset = set_iter->second;
		if (!curset->tags.empty()) {
			u_fprintf(ux_stdout, "LIST %S = ", curset->getName());
			for (comp_iter = curset->tags.begin() ; comp_iter != curset->tags.end() ; comp_iter++) {
				if (comp_iter->second) {
					CG3::CompositeTag *curcomptag = comp_iter->second;
					if (curcomptag->tags.size() == 1) {
						u_fprintf(ux_stdout, "%S ", curcomptag->tags.begin()->second->raw);
					} else {
						u_fprintf(ux_stdout, "(");
						std::map<uint32_t, CG3::Tag*>::iterator tag_iter;
						for (tag_iter = curcomptag->tags_map.begin() ; tag_iter != curcomptag->tags_map.end() ; tag_iter++) {
							u_fprintf(ux_stdout, "%S ", tag_iter->second->raw);
						}
						u_fprintf(ux_stdout, ") ");
					}
				}
			}
			u_fprintf(ux_stdout, "\n");
		}
	}
	u_fprintf(ux_stdout, "\n");
//*/

	u_fclose(ux_stdin);
	u_fclose(ux_stdout);
	u_fclose(ux_stderr);
	return status;
}
