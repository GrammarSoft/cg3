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

    U_MAIN_INIT_ARGS(argc, argv);

	argc = u_parseArgs(argc, argv, (int32_t)(sizeof(Options::options)/sizeof(Options::options[0])), Options::options);

    if (argc < 0) {
        fprintf(stderr, "%s: error in command line argument \"%s\"\n", argv[0], argv[-argc]);
    } else if (argc < 2) {
        argc = -1;
    }

    if (Options::options[Options::VERSION].doesOccur) {
        fprintf(stderr,
                "VISL CG-3 Disambiguator version %s.\n"
                "%s\n",
                CG3_VERSION_STRING, CG3_COPYRIGHT_STRING);
        return U_ZERO_ERROR;
    }

    if (argc < 0 || Options::options[Options::HELP1].doesOccur || Options::options[Options::HELP2].doesOccur) {
        /*
         * Broken into chucks because the C89 standard says the minimum
         * required supported string length is 509 bytes.
         */
        fprintf(stderr,
                "Usage: vislcg3 [OPTIONS] [FILES]\n"
                "  Reads the list of resource bundle source files and creates\n"
                "  binary version of reosurce bundles (.res files)\n");
        fprintf(stderr,
                "Options:\n"
                "  -h or -? or --help       this usage text\n"
                "  -q or --quiet            do not display warnings\n"
                "  -v or --verbose          print extra information when processing files\n"
                "  -V or --version          prints out version number and exits\n"
                "  -c or --copyright        include copyright notice\n");
        fprintf(stderr,
                "  -e or --encoding         encoding of source files\n"
                "  -d of --destdir          destination directory, followed by the path, defaults to\n"
                "  -s or --sourcedir        source directory for files followed by path, defaults to\n"
                "  -i or --icudatadir       directory for locating any needed intermediate data files,\n"
                "                           followed by path, defaults to\n");
        fprintf(stderr,
                "  -j or --write-java       write a Java ListResourceBundle for ICU4J, followed by optional encoding\n"
                "                           defaults to ASCII and \\uXXXX format.\n"
                "  -p or --package-name     For ICU4J: package name for writing the ListResourceBundle for ICU4J,\n"
                "                           defaults to com.ibm.icu.impl.data\n"
                "                           For ICU4C: Package name for the .res files on output. Specfiying\n"
                "                           'ICUDATA' defaults to the current ICU4C data name.\n");
        fprintf(stderr,
                "  -b or --bundle-name      bundle name for writing the ListResourceBundle for ICU4J,\n"
                "                           defaults to LocaleElements\n"
                "  -x or --write-xliff      write a XLIFF file for the resource bundle. Followed by an optional output file name.\n"
                "  -k or --strict           use pedantic parsing of syntax\n"
                /*added by Jing*/
                "  -l or --language         For XLIFF: language code compliant with ISO 639.\n");

        return argc < 0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    /* Initialize ICU */
    u_init(&status);
    if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
        fprintf(stderr, "Error: can not initialize ICU.  status = %s\n",
            u_errorName(status));
        exit(1);
    }
    status = U_ZERO_ERROR;

	CG3::Grammar *grammar = new CG3::Grammar;
	const char *codepage_grammar = "ISO-8859-1";

	if (Options::options[Options::CODEPAGE_GRAMMAR].doesOccur) {
		codepage_grammar = Options::options[Options::CODEPAGE_GRAMMAR].value;
	} else if (Options::options[Options::CODEPAGE_ALL].doesOccur) {
		codepage_grammar = Options::options[Options::CODEPAGE_ALL].value;
	}

	CG3::GrammarParser::parse_grammar_from_file(Options::options[Options::GRAMMAR].value, codepage_grammar, grammar);
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
	stdext::hash_map<UChar*, CG3::Set*>::iterator set_iter;
	for (set_iter = grammar->sets.begin() ; set_iter != grammar->sets.end() ; set_iter++) {
		std::vector<CG3::CompositeTag*>::iterator comp_iter;
		CG3::Set *curset = set_iter->second;
		std::wcout << "LIST " << set_iter->first << " = ";
		for (comp_iter = curset->tags.begin() ; comp_iter != curset->tags.end() ; comp_iter++) {
			CG3::CompositeTag *curcomptag = *comp_iter;
			if (curcomptag->num_tags == 1) {
				std::wcout << curcomptag->tags.front()->raw << " ";
			} else {
				std::wcout << "(";
				std::vector<CG3::Tag*>::iterator tag_iter;
				for (tag_iter = curcomptag->tags.begin() ; tag_iter != curcomptag->tags.end() ; tag_iter++) {
					std::wcout << (*tag_iter)->raw << " ";
				}
				std::wcout << ") ";
			}
		}
		std::wcout << " ;" << std::endl;
	}
	std::cout << std::endl;
//*/
	return status;
}
