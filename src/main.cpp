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
#include "Grammar.h"
#include "GrammarParser.h"
#include "GPRE2C.h"
#include "GrammarWriter.h"
#include "GrammarApplicator.h"

#include "version.h"

#include "options.h"
using namespace Options;
void GAppSetOpts(CG3::GrammarApplicator *applicator);

int main(int argc, char* argv[]) {
	UFILE *ux_stdin = 0;
	UFILE *ux_stdout = 0;
	UFILE *ux_stderr = 0;

	PACC::Timer *glob_timer = new PACC::Timer();
	PACC_TimeStamp main_timer = glob_timer->getCount();

	UErrorCode status = U_ZERO_ERROR;
	srand((uint32_t)time(0));

	fprintf(stderr, "VISL CG-3 Disambiguator version %u.%u.%u.%u\n",
		CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
	U_MAIN_INIT_ARGS(argc, argv);

	argc = u_parseArgs(argc, argv, NUM_OPTIONS, options);

	if (argc < 0) {
		fprintf(stderr, "%s: error in command line argument \"%s\"\n", argv[0], argv[-argc]);
		return argc;
	}

	if (options[VERSION].doesOccur) {
		fprintf(stderr, "%s\n", CG3_COPYRIGHT_STRING);
		return U_ZERO_ERROR;
	}

	if (!options[GRAMMAR].doesOccur && !options[HELP1].doesOccur && !options[HELP2].doesOccur) {
		fprintf(stderr, "Error: No grammar specified - cannot continue!\n");
		argc = -argc;
	}

	if (argc < 0 || options[HELP1].doesOccur || options[HELP2].doesOccur) {
		fprintf(stderr, "Usage: vislcg3 [OPTIONS]\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, " -h or -? or --help       Displays this list.\n");
		fprintf(stderr, " -V or --version          Prints version number.\n");
		fprintf(stderr, " -g or --grammar          Specifies the grammar file to use for disambiguation.\n");
		fprintf(stderr, " -p or --vislcg-compat    Tells the grammar compiler to be compatible with older VISLCG syntax.\n");
		fprintf(stderr, " --grammar-out            Writes the compiled grammar back out in textual form to a file.\n");
		//fprintf(stderr, " --grammar-bin            Writes the compiled grammar back out in binary form to a file.\n");
		fprintf(stderr, " --grammar-info           Writes the compiled grammar back out in textual form to a file, with lots of statistics and information.\n");
		fprintf(stderr, " --grammar-only           Compiles the grammar only.\n");
		fprintf(stderr, " --trace                  Prints debug output alongside with normal output.\n");
		fprintf(stderr, " --prefix                 Sets the prefix for mapping. Defaults to @.\n");
		fprintf(stderr, " --sections               Number of sections to run. Defaults to running all sections.\n");
		//fprintf(stderr, " --reorder                Rearranges rules so SELECTs are run first.\n");
		fprintf(stderr, " --single-run             Only runs each section once.\n");
		fprintf(stderr, " --no-mappings            Disables running any MAP, ADD, or REPLACE rules.\n");
		fprintf(stderr, " --no-corrections         Disables running any SUBSTITUTE or APPEND rules.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, " --num-windows            Number of windows to keep in before/ahead buffers. Defaults to 2.\n");
		fprintf(stderr, " --always-span            Forces all scanning tests to always span across window boundaries.\n");
		fprintf(stderr, " --soft-limit             Number of cohorts after which the SOFT-DELIMITERS kick in. Defaults to 300.\n");
		fprintf(stderr, " --hard-limit             Number of cohorts after which the window is delimited forcefully. Defaults to 500.\n");
		fprintf(stderr, " --no-magic-readings      Prevents running rules on magic readings.\n");
		fprintf(stderr, " --dep-allow-loops        Allows the creation of circular dependencies.\n");
		//fprintf(stderr, " --dep-delimit            Delimit via dependency information instead of DELIMITERS.\n");
		//fprintf(stderr, " --dep-reenum             Outputs the internal reenumeration of dependencies.\n");
		//fprintf(stderr, " --dep-humanize           Output dependency information in a more readable format.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, " -O or --stdout           A file to print output to instead of stdout.\n");
		fprintf(stderr, " -I or --stdin            A file to read input from instead of stdin.\n");
		fprintf(stderr, " -E or --stderr           A file to print errors to instead of stderr.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, " -C or --codepage-all     The codepage to use for grammar, input, and output streams. Defaults to ISO-8859-1.\n");
		fprintf(stderr, " --codepage-grammar       Codepage to use for grammar. Overrides --codepage-all.\n");
		fprintf(stderr, " --codepage-input         Codepage to use for input. Overrides --codepage-all.\n");
		fprintf(stderr, " --codepage-output        Codepage to use for output and errors. Overrides --codepage-all.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, " -L or --locale-all       The locale to use for grammar, input, and output streams. Defaults to en_US_POSIX.\n");
		fprintf(stderr, " --locale-grammar         Locale to use for grammar. Overrides --locale-all.\n");
		fprintf(stderr, " --locale-input           Locale to use for input. Overrides --locale-all.\n");
		fprintf(stderr, " --locale-output          Locale to use for output and errors. Overrides --locale-all.\n");

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

	CG3::Grammar *grammar = new CG3::Grammar();
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

	if (!options[STDOUT].doesOccur) {
		ux_stdout = u_finit(stdout, locale_output, codepage_input);
	} else {
		ux_stdout = u_fopen(options[STDOUT].value, "wb", locale_output, codepage_output);
	}
	if (!ux_stdout) {
		fprintf(stderr, "Error: Failed to open the output stream for writing!\n");
		return -1;
	}

	if (!options[STDERR].doesOccur) {
		ux_stderr = u_finit(stderr, locale_output, codepage_input);
	} else {
		ux_stderr = u_fopen(options[STDERR].value, "wb", locale_output, codepage_output);
	}
	if (!ux_stdout) {
		fprintf(stderr, "Error: Failed to open the error stream for writing!\n");
		return -1;
	}

	if (!options[STDIN].doesOccur) {
		ux_stdin = u_finit(stdin, locale_input, codepage_input);
	} else {
		struct stat info;
		int serr = stat(options[STDIN].value, &info);
		if (serr) {
			fprintf(stderr, "Error: Cannot stat %s due to error %d!\n", options[STDIN].value, serr);
			return serr;
		}
		ux_stdin = u_fopen(options[STDIN].value, "rb", locale_input, codepage_input);
	}
	if (!ux_stdin) {
		fprintf(stderr, "Error: Failed to open the input stream for reading!\n");
		return -1;
	}

	CG3::IGrammarParser *parser = 0;
	if (options[RE2C].doesOccur) {
		fprintf(stderr, "Info: Using experimental RE2C parser.\n");
		parser = new CG3::GPRE2C(ux_stdin, ux_stdout, ux_stderr);
	}
	else {
		parser = new CG3::GrammarParser(ux_stdin, ux_stdout, ux_stderr);
	}
	grammar->ux_stderr = ux_stderr;
	parser->setResult(grammar);
	parser->setCompatible(options[VISLCGCOMPAT].doesOccur != 0);

	std::cerr << "Initialization took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
	main_timer = glob_timer->getCount();

	if (parser->parse_grammar_from_file(options[GRAMMAR].value, locale_grammar, codepage_grammar)) {
		u_fprintf(ux_stderr, "Error: Grammar could not be parsed - exiting!\n");
		return -1;
	}

	if (options[MAPPING_PREFIX].doesOccur) {
		grammar->mapping_prefix = options[MAPPING_PREFIX].value[0];
	}
	grammar->reindex();

	delete parser;
	parser = 0;
	
	std::cerr << "Parsing grammar took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
	main_timer = glob_timer->getCount();

	if (grammar->has_dep) {
		std::cerr << "Grammar has dependency rules: Yes" << std::endl;
	}
	else {
		std::cerr << "Grammar has dependency rules: No" << std::endl;
	}

	CG3::GrammarWriter *writer = 0;
	CG3::GrammarApplicator *applicator = 0;

	if (options[REORDER].doesOccur) {
		for (uint32_t j=0;j<grammar->rules.size();j++) {
			if (grammar->rules[j]->type == K_IFF) {
				grammar->rules[j]->quality = 4.0;
			}
			else if (grammar->rules[j]->type == K_SELECT) {
				grammar->rules[j]->quality = 3.0;
			}
			else if (grammar->rules[j]->type == K_REMOVE) {
				grammar->rules[j]->quality = 2.0;
			}
			else {
				grammar->rules[j]->quality = 1.0;
			}
		}
		for (uint32_t i=0;i<grammar->sections.size()-1;i++) {
			std::sort(&grammar->rules[grammar->sections[i]], &grammar->rules[grammar->sections[i+1]-1], CG3::Rule::cmp_quality);
		}
	}

	if (!options[CHECK_ONLY].doesOccur && !options[GRAMMAR_ONLY].doesOccur) {
		grammar->trim();

		applicator = new CG3::GrammarApplicator(ux_stdin, ux_stdout, ux_stderr);
		applicator->setGrammar(grammar);
		GAppSetOpts(applicator);
		applicator->runGrammarOnText(ux_stdin, ux_stdout);

		std::cerr << "Applying grammar on input took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
		main_timer = glob_timer->getCount();
	}

	if (options[GRAMMAR_INFO].doesOccur) {
		UFILE *gout = u_fopen(options[GRAMMAR_INFO].value, "wb", locale_output, codepage_output);
		if (gout) {
			for (uint32_t j=0;j<grammar->rules.size();j++) {
				grammar->rules[j]->reset();
			}

			if (!options[CHECK_ONLY].doesOccur && !options[GRAMMAR_ONLY].doesOccur) {
				u_frewind(ux_stdin);
				grammar->trim();

				applicator = new CG3::GrammarApplicator(ux_stdin, ux_stdout, ux_stderr);
				applicator->setGrammar(grammar);
				GAppSetOpts(applicator);
				applicator->enableStatistics();
				applicator->runGrammarOnText(ux_stdin, ux_stdout);

				std::cerr << "Applying context-sorted grammar on input took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
				main_timer = glob_timer->getCount();
			}

			/*
			for (uint32_t j=0;j<grammar->rules.size();j++) {
				grammar->rules[j]->reweight();
			}
			//*/
			for (uint32_t i=0;i<grammar->sections.size()-1;i++) {
				std::sort(&grammar->rules[grammar->sections[i]], &grammar->rules[grammar->sections[i+1]-1], CG3::Rule::cmp_quality);
			}

			writer = new CG3::GrammarWriter(ux_stdin, ux_stdout, ux_stderr);
			writer->setGrammar(grammar);
			writer->statistics = true;
			writer->write_grammar_to_ufile_text(gout);

			std::cerr << "Writing textual grammar with statistics took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
			main_timer = glob_timer->getCount();
		} else {
			std::cerr << "Could not write grammar to " << options[GRAMMAR_INFO].value << std::endl;
		}

		if (!options[CHECK_ONLY].doesOccur && !options[GRAMMAR_ONLY].doesOccur) {
			u_frewind(ux_stdin);
			grammar->trim();

			applicator = new CG3::GrammarApplicator(ux_stdin, ux_stdout, ux_stderr);
			applicator->setGrammar(grammar);
			GAppSetOpts(applicator);
			applicator->enableStatistics();
			applicator->runGrammarOnText(ux_stdin, ux_stdout);

			std::cerr << "Applying fully-sorted grammar on input took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
			main_timer = glob_timer->getCount();
		}
	}

	if (options[GRAMMAR_OUT].doesOccur) {
		UFILE *gout = u_fopen(options[GRAMMAR_OUT].value, "w", locale_output, codepage_output);
		if (gout) {
			writer = new CG3::GrammarWriter(ux_stdin, ux_stdout, ux_stderr);
			writer->setGrammar(grammar);
			writer->write_grammar_to_ufile_text(gout);

			std::cerr << "Writing textual grammar took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
			main_timer = glob_timer->getCount();
		} else {
			std::cerr << "Could not write grammar to " << options[GRAMMAR_OUT].value << std::endl;
		}
	}

	if (options[GRAMMAR_BIN].doesOccur) {
		FILE *gout = fopen(options[GRAMMAR_BIN].value, "wb");
		if (gout) {
			writer = new CG3::GrammarWriter(ux_stdin, ux_stdout, ux_stderr);
			writer->setGrammar(grammar);
			writer->write_grammar_to_file_binary(gout);

			std::cerr << "Writing binary grammar took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
			main_timer = glob_timer->getCount();
		} else {
			std::cerr << "Could not write grammar to " << options[GRAMMAR_BIN].value << std::endl;
		}
	}

	u_fclose(ux_stdin);
	u_fclose(ux_stdout);
	u_fclose(ux_stderr);

	delete grammar;
	delete writer;
	delete applicator;

	std::cerr << "Cleanup took " << glob_timer->getValueFrom(main_timer) << " seconds." << std::endl;
	delete glob_timer;

	return status;
}

void GAppSetOpts(CG3::GrammarApplicator *applicator) {
	if (options[ALWAYS_SPAN].doesOccur) {
		applicator->always_span = true;
	}
	applicator->apply_mappings = true;
	if (options[NOMAPPINGS].doesOccur) {
		applicator->apply_mappings = false;
	}
	applicator->apply_corrections = true;
	if (options[NOCORRECTIONS].doesOccur) {
		applicator->apply_corrections = false;
	}
	if (options[TRACE].doesOccur) {
		applicator->trace = true;
	}
	/*
	if (options[TRACE_ALL].doesOccur) {
		applicator->trace = true;
		applicator->trace_all = true;
	}
	//*/
	if (options[SINGLERUN].doesOccur) {
		applicator->single_run = true;
	}
	if (options[GRAMMAR_INFO].doesOccur) {
		applicator->enableStatistics();
	}
	if (options[SECTIONS].doesOccur) {
		applicator->sections = abs(atoi(options[SECTIONS].value));
	}
	if (options[NUM_WINDOWS].doesOccur) {
		applicator->num_windows = abs(atoi(options[NUM_WINDOWS].value));
	}
	if (options[SOFT_LIMIT].doesOccur) {
		applicator->soft_limit = abs(atoi(options[SOFT_LIMIT].value));
	}
	if (options[HARD_LIMIT].doesOccur) {
		applicator->hard_limit = abs(atoi(options[HARD_LIMIT].value));
	}
	if (options[DEP_REENUM].doesOccur) {
		applicator->dep_reenum = true;
	}
	if (options[DEP_HUMANIZE].doesOccur) {
		applicator->dep_reenum = true;
		applicator->dep_humanize = true;
	}
	if (options[DEP_ORIGINAL].doesOccur) {
		applicator->dep_original = true;
	}
	if (options[DEP_ALLOW_LOOPS].doesOccur) {
		applicator->dep_block_loops = false;
	}
	if (options[MAGIC_READINGS].doesOccur) {
		applicator->allow_magic_readings = false;
	}
}
