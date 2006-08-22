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
#include "uoptions.h"
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
		CODEPAGE_ALL,
		CODEPAGE_GRAMMAR,
		CODEPAGE_INPUT,
		CODEPAGE_OUTPUT,
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
		UOPTION_DEF("codepage-all",			'C', UOPT_REQUIRES_ARG),
		UOPTION_DEF("codepage-grammar",		NULL, UOPT_REQUIRES_ARG),
		UOPTION_DEF("codepage-input",		NULL, UOPT_REQUIRES_ARG),
		UOPTION_DEF("codepage-output",		NULL, UOPT_REQUIRES_ARG)
	};
}

int main(int argc, char* argv[]) {
    UErrorCode status = U_ZERO_ERROR;
	srand((unsigned int)time(0));

    U_MAIN_INIT_ARGS(argc, argv);

	argc = u_parseArgs(argc, argv, (int32_t)(sizeof(Options::options)/sizeof(Options::options[0])), Options::options);

    if (argc < 0) {
        fprintf(stderr, "%s: error in command line argument \"%s\"\n", argv[0], argv[-argc]);
    }

    if (Options::options[Options::VERSION].doesOccur) {
        fprintf(stderr,
                "VISL CG-3 Disambiguator version %s.\n"
                "%s\n",
                CG3_VERSION_STRING, CG3_COPYRIGHT_STRING);
        return U_ZERO_ERROR;
    }

	if (!Options::options[Options::GRAMMAR].doesOccur) {
		std::cerr << "Error: No grammar specified - cannot continue!" << std::endl;
		argc = -argc;
	}

    if (argc < 0 || Options::options[Options::HELP1].doesOccur || Options::options[Options::HELP2].doesOccur) {
        std::cerr << "Usage: vislcg3 [OPTIONS] [FILES]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << " -h or -? or --help       Displays this list." << std::endl;
        std::cerr << " -V or --version          Prints version number." << std::endl;
        std::cerr << " -g or --grammar          Specifies the grammar file to use for disambiguation." << std::endl;
        std::cerr << " -C or --codepage-all     The codepage to use for grammar, input, and output streams. Defaults to ISO-8859-1." << std::endl;
        std::cerr << " --codepage-grammar       Codepage to use for grammar. Overwrites --codepage-all." << std::endl;
        std::cerr << " --codepage-input         Codepage to use for input. Overwrites --codepage-all." << std::endl;
        std::cerr << " --codepage-output        Codepage to use for output. Overwrites --codepage-all." << std::endl;
        
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
	const char *codepage_input   = "ISO-8859-1";
	const char *codepage_output  = "ISO-8859-1";

	if (Options::options[Options::CODEPAGE_GRAMMAR].doesOccur) {
		codepage_grammar = Options::options[Options::CODEPAGE_GRAMMAR].value;
	} else if (Options::options[Options::CODEPAGE_ALL].doesOccur) {
		codepage_grammar = Options::options[Options::CODEPAGE_ALL].value;
	}

	if (Options::options[Options::CODEPAGE_INPUT].doesOccur) {
		codepage_input = Options::options[Options::CODEPAGE_INPUT].value;
	} else if (Options::options[Options::CODEPAGE_ALL].doesOccur) {
		codepage_input = Options::options[Options::CODEPAGE_ALL].value;
	}

	if (Options::options[Options::CODEPAGE_OUTPUT].doesOccur) {
		codepage_output = Options::options[Options::CODEPAGE_OUTPUT].value;
	} else if (Options::options[Options::CODEPAGE_ALL].doesOccur) {
		codepage_output = Options::options[Options::CODEPAGE_ALL].value;
	}

	CG3::GrammarParser::parse_grammar_from_file(Options::options[Options::GRAMMAR].value, codepage_grammar, grammar);

	UFILE *input = 0;
	if (argc < 2 || strcmp(argv[argc-1], "-") == 0) {
		input = u_finit(stdin, NULL, codepage_input);
	} else {
		input = u_fopen(argv[argc-1], "r", NULL, codepage_input);
	}

//*
	std::cout << "DELIMITERS = ";
	stdext::hash_map<UChar*, unsigned long>::iterator iter;
	for(iter = grammar->delimiters.begin() ; iter != grammar->delimiters.end() ; iter++ ) {
		std::wcout << " " << iter->first;
	}
	std::cout << " ;" << std::endl;

	std::cout << "PREFERRED-TARGETS = ";
	for(iter = grammar->preferred_targets.begin() ; iter != grammar->preferred_targets.end() ; iter++ ) {
		std::wcout << " " << iter->first;
	}
	std::cout << " ;" << std::endl;

	std::cout << "SETS" << std::endl;
	stdext::hash_map<unsigned long, CG3::Set*>::iterator set_iter;
	for (set_iter = grammar->sets.begin() ; set_iter != grammar->sets.end() ; set_iter++) {
		stdext::hash_map<unsigned long, CG3::CompositeTag*>::iterator comp_iter;
		CG3::Set *curset = set_iter->second;
		if (!curset->tags.empty()) {
			std::wcout << "LIST " << curset->getName() << " = ";
			for (comp_iter = curset->tags.begin() ; comp_iter != curset->tags.end() ; comp_iter++) {
				if (comp_iter->second) {
					CG3::CompositeTag *curcomptag = comp_iter->second;
					if (curcomptag->tags.size() == 1) {
						std::wcout << curcomptag->tags.begin()->second->raw << " ";
					} else {
						std::wcout << "(";
						std::map<unsigned long, CG3::Tag*>::iterator tag_iter;
						for (tag_iter = curcomptag->tags_map.begin() ; tag_iter != curcomptag->tags_map.end() ; tag_iter++) {
							std::wcout << tag_iter->second->raw << " ";
						}
						std::wcout << ") ";
					}
				}
			}
			std::wcout << " ;" << std::endl;
		}
	}
	std::cout << std::endl;
//*/
	return status;
}
