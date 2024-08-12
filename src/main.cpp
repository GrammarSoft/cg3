/*
* Copyright (C) 2007-2024, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "stdafx.hpp"
#include "Grammar.hpp"
#include "TextualParser.hpp"
#include "GrammarWriter.hpp"
#include "BinaryGrammar.hpp"
#include "GrammarApplicator.hpp"
#include "version.hpp"

#include "options.hpp"
using namespace Options;
using namespace CG3;
void GAppSetOpts(GrammarApplicator& applicator, UConverter* conv);

int main(int argc, char* argv[]) {
	clock_t main_timer = clock();

	UErrorCode status = U_ZERO_ERROR;
	srand(UI32(time(0)));

	argc = u_parseArgs(argc, argv, NUM_OPTIONS, options.data());
	FILE* out = stderr;

	parse_opts_env("CG3_DEFAULT", options_default);
	parse_opts_env("CG3_OVERRIDE", options_override);
	for (size_t i = 0; i < options.size(); ++i) {
		if (options_default[i].doesOccur && !options[i].doesOccur) {
			options[i] = options_default[i];
		}
		if (options_override[i].doesOccur) {
			options[i] = options_override[i];
		}
	}

	if (options[VERSION_TOO_OLD].doesOccur) {
		std::cout << CG3_TOO_OLD << std::endl;
		return 0;
	}

	if (options[VERSION].doesOccur || options[HELP1].doesOccur || options[HELP2].doesOccur) {
		out = stdout;
		// Keep the invocation vislcg3 --version | grep -Eo '[0-9]+$' holy so that it only outputs the revision regardless of other flags.
		fprintf(out, "VISL CG-3 Disambiguator version %u.%u.%u.%u\n", CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
	}

	if (argc < 0) {
		fprintf(stderr, "%s: error in command line argument \"%s\"\n", argv[0], argv[-argc]);
		return argc;
	}

	if (options[VERSION].doesOccur) {
		fprintf(out, "%s\n", CG3_COPYRIGHT_STRING);
		return U_ZERO_ERROR;
	}

	if (!options[GRAMMAR].doesOccur && !options[HELP1].doesOccur && !options[HELP2].doesOccur) {
		fprintf(stderr, "Error: No grammar specified - cannot continue!\n");
		argc = -argc;
	}

	if (argc < 0 || options[HELP1].doesOccur || options[HELP2].doesOccur) {
		fprintf(out, "Usage: vislcg3 [OPTIONS]\n");
		fprintf(out, "\n");
		fprintf(out, "Environment variable:\n");
		fprintf(out, " CG3_DEFAULT: Sets default cmdline options, which the actual passed options will override.\n");
		fprintf(out, " CG3_OVERRIDE: Sets forced cmdline options, which will override any passed option.\n");
		fprintf(out, "\n");
		fprintf(out, "Options:\n");

		size_t longest = 0;
		for (uint32_t i = 0; i < NUM_OPTIONS; i++) {
			if (!options[i].description.empty()) {
				size_t len = strlen(options[i].longName);
				longest = std::max(longest, len);
			}
		}
		for (uint32_t i = 0; i < NUM_OPTIONS; i++) {
			if (!options[i].description.empty()) {
				fprintf(out, " ");
				if (options[i].shortName) {
					fprintf(out, "-%c,", options[i].shortName);
				}
				else {
					fprintf(out, "   ");
				}
				fprintf(out, " --%s", options[i].longName);
				size_t ldiff = longest - strlen(options[i].longName);
				while (ldiff--) {
					fprintf(out, " ");
				}
				fprintf(out, "  %s", options[i].description.c_str());
				fprintf(out, "\n");
			}
		}

		return argc < 0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
	}

	fflush(out);
	fflush(stderr);

	if (options[SHOW_UNUSED_SETS].doesOccur || options[SHOW_SET_HASHES].doesOccur || options[DUMP_AST].doesOccur) {
		options[GRAMMAR_ONLY].doesOccur = true;
	}

	if (options[GRAMMAR_ONLY].doesOccur) {
		if (!options[VERBOSE].doesOccur) {
			options[VERBOSE].doesOccur = true;
		}
	}

	if (options[QUIET].doesOccur) {
		options[VERBOSE].doesOccur = false;
	}
	if (options[VERBOSE].doesOccur && !options[VERBOSE].value.empty() && options[VERBOSE].value == "0") {
		options[VERBOSE].doesOccur = false;
	}

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}
	status = U_ZERO_ERROR;

	const char* codepage_cli = ucnv_getDefaultName();
	ucnv_setDefaultName("UTF-8");

	if (options[CODEPAGE_GLOBAL].doesOccur || options[CODEPAGE_INPUT].doesOccur || options[CODEPAGE_OUTPUT].doesOccur || options[CODEPAGE_GRAMMAR].doesOccur) {
		std::cerr << "Warning: The -C and --codepage-* option are deprecated and now default to UTF-8" << std::endl;
	}

	uloc_setDefault("en_US_POSIX", &status);

	UConverter* conv = ucnv_open(codepage_cli, &status);

	std::ostream* ux_stdout = &std::cout;
	std::unique_ptr<std::ofstream> _ux_stdout;
	if (options[STDOUT].doesOccur) {
		_ux_stdout.reset(new std::ofstream(options[STDOUT].value, std::ios::binary));

		if (!_ux_stdout || _ux_stdout->bad()) {
			std::cerr << "Error: Failed to open the output stream for writing!" << std::endl;
			CG3Quit(1);
		}
		ux_stdout = _ux_stdout.get();
	}

	std::ostream* ux_stderr = &std::cerr;
	std::unique_ptr<std::ofstream> _ux_stderr;
	if (options[STDERR].doesOccur) {
		_ux_stderr.reset(new std::ofstream(options[STDERR].value, std::ios::binary));

		if (!_ux_stderr || _ux_stderr->bad()) {
			std::cerr << "Error: Failed to open the error stream for writing!" << std::endl;
			CG3Quit(1);
		}
		ux_stderr = _ux_stderr.get();
	}

	std::istream* ux_stdin = &std::cin;
	std::unique_ptr<std::ifstream> _ux_stdin;
	if (options[STDIN].doesOccur) {
		struct stat info;
		int serr = stat(options[STDIN].value.c_str(), &info);
		if (serr) {
			std::cerr << "Error: Cannot stat " << options[STDIN].value << " due to error " << serr << "!" << std::endl;
			CG3Quit(1);
		}

		_ux_stdin.reset(new std::ifstream(options[STDIN].value, std::ios::binary));

		if (!_ux_stdin || _ux_stdin->bad()) {
			std::cerr << "Error: Failed to open the input stream for reading!" << std::endl;
			CG3Quit(1);
		}
		ux_stdin = _ux_stdin.get();
	}

	Grammar grammar;

	if (options[SHOW_TAG_HASHES].doesOccur) {
		Tag::dump_hashes_out = ux_stderr;
	}
	if (options[SHOW_SET_HASHES].doesOccur) {
		Set::dump_hashes_out = ux_stderr;
	}

	std::unique_ptr<IGrammarParser> parser;
	FILE* input = fopen(options[GRAMMAR].value.c_str(), "rb");
	if (!input) {
		std::cerr << "Error: Error opening " << options[GRAMMAR].value << " for reading!" << std::endl;
		CG3Quit(1);
	}
	if (fread(&cbuffers[0][0], 1, 4, input) != 4) {
		std::cerr << "Error: Error reading first 4 bytes from grammar!" << std::endl;
		CG3Quit(1);
	}
	fclose(input);

	if (is_cg3b(cbuffers[0])) {
		if (options[VERBOSE].doesOccur) {
			std::cerr << "Info: Binary grammar detected." << std::endl;
		}
		if (options[DUMP_AST].doesOccur) {
			std::cerr << "Error: --dump-ast is for textual grammars only!" << std::endl;
			CG3Quit(1);
		}
		parser.reset(new BinaryGrammar(grammar, *ux_stderr));
	}
	else {
		parser.reset(new TextualParser(grammar, *ux_stderr, options[DUMP_AST].doesOccur != 0));
	}
	if (options[VERBOSE].doesOccur) {
		if (!options[VERBOSE].value.empty()) {
			uint32_t verbosity_level = std::stoul(options[VERBOSE].value);
			parser->setVerbosity(verbosity_level);
			grammar.verbosity_level = verbosity_level;
		}
		else {
			parser->setVerbosity(1);
			grammar.verbosity_level = 1;
		}
	}
	grammar.ux_stderr = ux_stderr;
	grammar.ux_stdout = ux_stdout;
	parser->setCompatible(options[VISLCGCOMPAT].doesOccur != 0);

	if (options[VERBOSE].doesOccur) {
		std::cerr << "Initialization took " << (clock() - main_timer) / (double)CLOCKS_PER_SEC << " seconds." << std::endl;
	}
	main_timer = clock();

	if (options[NRULES].doesOccur) {
		ucnv_reset(conv);
		auto sn = options[NRULES].value.size();
		UString buf(sn * 3, 0);
		buf.resize(ucnv_toUChars(conv, &buf[0], SI32(buf.size()), options[NRULES].value.c_str(), SI32(sn), &status));
		parser->nrules = uregex_open(buf.c_str(), SI32(buf.size()), 0, nullptr, &status);
		if (status != U_ZERO_ERROR) {
			u_fprintf(std::cerr, "Error: uregex_open returned %s trying to parse --nrules %S\n", u_errorName(status), buf.c_str());
			CG3Quit(1);
		}
	}

	if (options[NRULES_INV].doesOccur) {
		ucnv_reset(conv);
		auto sn = options[NRULES_INV].value.size();
		UString buf(sn * 3, 0);
		buf.resize(ucnv_toUChars(conv, &buf[0], SI32(buf.size()), options[NRULES_INV].value.c_str(), SI32(sn), &status));
		parser->nrules_inv = uregex_open(buf.c_str(), SI32(buf.size()), 0, nullptr, &status);
		if (status != U_ZERO_ERROR) {
			u_fprintf(std::cerr, "Error: uregex_open returned %s trying to parse --nrules-v %S\n", u_errorName(status), buf.c_str());
			CG3Quit(1);
		}
	}

	std::unique_ptr<Profiler> profiler;
	if (options[PROFILING].doesOccur) {
		profiler.reset(new Profiler);
		dynamic_cast<TextualParser*>(parser.get())->profiler = profiler.get();
	}

	if (parser->parse_grammar(options[GRAMMAR].value.c_str())) {
		std::cerr << "Error: Grammar could not be parsed - exiting!" << std::endl;
		CG3Quit(1);
	}

	if (!grammar.cmdargs.empty()) {
		auto args = grammar.cmdargs;
		args.push_back(0);
		parse_opts(args.data(), grammar_options_default);
	}
	if (!grammar.cmdargs_override.empty()) {
		auto args = grammar.cmdargs_override;
		args.push_back(0);
		parse_opts(args.data(), grammar_options_override);
	}
	for (size_t i = 0; i < options.size(); ++i) {
		if (grammar_options_default[i].doesOccur && !options[i].doesOccur) {
			options[i] = grammar_options_default[i];
		}
		if (grammar_options_override[i].doesOccur && !options_override[i].doesOccur) {
			options[i] = grammar_options_override[i];
		}
	}

	if (options[DUMP_AST].doesOccur) {
		dynamic_cast<TextualParser*>(parser.get())->print_ast(*ux_stdout);
	}
	if (options[PROFILING].doesOccur) {
		auto& buf = profiler->buf;
		buf.str("");
		buf.clear();
		dynamic_cast<TextualParser*>(parser.get())->print_ast(buf);
		auto sz = profiler->addString(buf.str());
		profiler->grammar_ast = sz;
	}

	if (options[MAPPING_PREFIX].doesOccur) {
		ucnv_reset(conv);
		auto sn = options[MAPPING_PREFIX].value.size();
		UString buf(sn * 3, 0);
		ucnv_toUChars(conv, &buf[0], SI32(buf.size()), options[MAPPING_PREFIX].value.c_str(), SI32(sn), &status);
		if (grammar.is_binary && grammar.mapping_prefix != buf[0]) {
			std::cerr << "Error: Mapping prefix must match the one used for compiling the binary grammar!" << std::endl;
			CG3Quit(1);
		}
		grammar.mapping_prefix = buf[0];
	}
	if (options[VERBOSE].doesOccur) {
		std::cerr << "Reindexing grammar..." << std::endl;
	}
	grammar.reindex(options[SHOW_UNUSED_SETS].doesOccur == 1, options[SHOW_TAGS].doesOccur == 1);

	parser.reset();

	if (options[VERBOSE].doesOccur) {
		std::cerr << "Parsing grammar took " << (clock() - main_timer) / (double)CLOCKS_PER_SEC << " seconds." << std::endl;
	}
	main_timer = clock();

	if (options[VERBOSE].doesOccur) {
		std::cerr << "Grammar has " << grammar.sections.size() << " sections, " << grammar.templates.size() << " templates, " << grammar.rule_by_number.size() << " rules, " << grammar.sets_list.size() << " sets, " << grammar.single_tags.size() << " tags." << std::endl;
		if (grammar.rules_any) {
			std::cerr << grammar.rules_any->size() << " rules cannot be skipped by index." << std::endl;
		}
		if (grammar.has_dep) {
			std::cerr << "Grammar has dependency rules." << std::endl;
		}
		if (grammar.has_relations) {
			std::cerr << "Grammar has relation rules." << std::endl;
		}
	}

	if (options[PROFILING].doesOccur && options[GRAMMAR_ONLY].doesOccur) {
		std::cerr << "Error: Cannot gather profiling data with no input to run grammar on." << std::endl;
		CG3Quit(1);
	}

	if (!options[GRAMMAR_ONLY].doesOccur) {
		GrammarApplicator applicator(*ux_stderr);
		applicator.setGrammar(&grammar);
		GAppSetOpts(applicator, conv);
		if (options[PROFILING].doesOccur) {
			applicator.profiler = profiler.get();
		}
		applicator.runGrammarOnText(*ux_stdin, *ux_stdout);

		if (options[VERBOSE].doesOccur) {
			std::cerr << "Applying grammar on input took " << (clock() - main_timer) / (double)CLOCKS_PER_SEC << " seconds." << std::endl;
		}
		main_timer = clock();
	}

	if (options[GRAMMAR_OUT].doesOccur) {
		std::ofstream gout(options[GRAMMAR_OUT].value, std::ios::binary);
		if (gout) {
			GrammarWriter writer(grammar, *ux_stderr);
			writer.writeGrammar(gout);

			if (options[VERBOSE].doesOccur) {
				std::cerr << "Writing textual grammar took " << (clock() - main_timer) / (double)CLOCKS_PER_SEC << " seconds." << std::endl;
			}
			main_timer = clock();
		}
		else {
			std::cerr << "Could not write grammar to " << options[GRAMMAR_OUT].value << std::endl;
		}
	}

	if (options[GRAMMAR_BIN].doesOccur) {
		FILE* gout = fopen(options[GRAMMAR_BIN].value.c_str(), "wb");
		if (gout) {
			BinaryGrammar writer(grammar, *ux_stderr);
			writer.writeBinaryGrammar(gout);

			if (options[VERBOSE].doesOccur) {
				std::cerr << "Writing binary grammar took " << (clock() - main_timer) / (double)CLOCKS_PER_SEC << " seconds." << std::endl;
			}
			main_timer = clock();
		}
		else {
			std::cerr << "Could not write grammar to " << options[GRAMMAR_BIN].value << std::endl;
		}
	}

	if (options[PROFILING].doesOccur) {
		profiler->write(options[PROFILING].value.c_str());
	}

	ucnv_close(conv);
	u_cleanup();

	if (options[VERBOSE].doesOccur) {
		std::cerr << "Cleanup took " << (clock() - main_timer) / (double)CLOCKS_PER_SEC << " seconds." << std::endl;
	}

	return status;
}

void GAppSetOpts(GrammarApplicator& applicator, UConverter* conv) {
	if (options[ALWAYS_SPAN].doesOccur) {
		applicator.always_span = true;
	}
	applicator.unicode_tags = false;
	if (options[UNICODE_TAGS].doesOccur) {
		applicator.unicode_tags = true;
	}
	applicator.unique_tags = false;
	if (options[UNIQUE_TAGS].doesOccur) {
		applicator.unique_tags = true;
	}
	applicator.apply_mappings = true;
	if (options[NOMAPPINGS].doesOccur) {
		applicator.apply_mappings = false;
	}
	applicator.apply_corrections = true;
	if (options[NOCORRECTIONS].doesOccur) {
		applicator.apply_corrections = false;
	}
	applicator.no_before_sections = false;
	if (options[NOBEFORESECTIONS].doesOccur) {
		applicator.no_before_sections = true;
	}
	applicator.no_sections = false;
	if (options[NOSECTIONS].doesOccur) {
		applicator.no_sections = true;
	}
	applicator.no_after_sections = false;
	if (options[NOAFTERSECTIONS].doesOccur) {
		applicator.no_after_sections = true;
	}
	applicator.unsafe = false;
	if (options[UNSAFE].doesOccur) {
		applicator.unsafe = true;
	}
	if (options[ORDERED].doesOccur) {
		applicator.ordered = true;
	}
	if (options[TRACE].doesOccur) {
		applicator.trace = true;
		if (!options[TRACE].value.empty()) {
			GAppSetOpts_ranged(options[TRACE].value.c_str(), applicator.trace_rules, false);
		}
	}
	if (options[TRACE_NAME_ONLY].doesOccur) {
		applicator.trace = true;
		applicator.trace_name_only = true;
	}
	if (options[TRACE_NO_REMOVED].doesOccur) {
		applicator.trace = true;
		applicator.trace_no_removed = true;
	}
	if (options[TRACE_ENCL].doesOccur) {
		applicator.trace = true;
		applicator.trace_encl = true;
	}
	if (options[PIPE_DELETED].doesOccur) {
		applicator.pipe_deleted = true;
	}
	if (options[DRYRUN].doesOccur) {
		applicator.dry_run = true;
	}
	if (options[SINGLERUN].doesOccur) {
		applicator.section_max_count = 1;
	}
	if (options[MAXRUNS].doesOccur) {
		applicator.section_max_count = std::stoul(options[MAXRUNS].value);
	}
	if (options[SECTIONS].doesOccur) {
		GAppSetOpts_ranged(options[SECTIONS].value.c_str(), applicator.sections);
	}
	if (options[RULES].doesOccur) {
		GAppSetOpts_ranged(options[RULES].value.c_str(), applicator.valid_rules);
	}
	if (options[RULE].doesOccur) {
		if (options[RULE].value[0] >= '0' && options[RULE].value[0] <= '9') {
			applicator.valid_rules.push_back(std::stoi(options[RULE].value));
		}
		else {
			UErrorCode status = U_ZERO_ERROR;
			auto sn = options[RULE].value.size();
			UString buf(sn * 3, 0);
			ucnv_reset(conv);
			ucnv_toUChars(conv, &buf[0], SI32(sn * 3), options[RULE].value.c_str(), SI32(sn), &status);

			for (auto rule : applicator.grammar->rule_by_number) {
				if (rule->name == buf) {
					applicator.valid_rules.push_back(rule->number);
				}
			}
		}
	}
	if (options[DEBUG_RULES].doesOccur) {
		GAppSetOpts_ranged(options[DEBUG_RULES].value.c_str(), applicator.debug_rules, false);
	}
	if (options[VERBOSE].doesOccur) {
		if (!options[VERBOSE].value.empty()) {
			applicator.verbosity_level = std::stoul(options[VERBOSE].value);
		}
		else {
			applicator.verbosity_level = 1;
		}
	}
	if (options[DODEBUG].doesOccur) {
		if (!options[DODEBUG].value.empty()) {
			applicator.debug_level = std::stoul(options[DODEBUG].value);
		}
		else {
			applicator.debug_level = 1;
		}
		std::cerr << "Debug level set to " << applicator.debug_level << std::endl;
	}
	if (options[NUM_WINDOWS].doesOccur) {
		applicator.num_windows = std::stoul(options[NUM_WINDOWS].value);
	}
	if (options[SOFT_LIMIT].doesOccur) {
		applicator.soft_limit = std::stoul(options[SOFT_LIMIT].value);
	}
	if (options[HARD_LIMIT].doesOccur) {
		applicator.hard_limit = std::stoul(options[HARD_LIMIT].value);
	}
	if (options[TEXT_DELIMIT].doesOccur) {
		UString rx{ STR_TEXTDELIM_DEFAULT };
		if (!options[TEXT_DELIMIT].value.empty()) {
			UErrorCode status = U_ZERO_ERROR;
			auto sn = options[TEXT_DELIMIT].value.size();
			UString buf(sn * 3, 0);
			ucnv_reset(conv);
			auto len = ucnv_toUChars(conv, &buf[0], SI32(sn * 3), options[TEXT_DELIMIT].value.c_str(), SI32(sn), &status);
			rx.assign(buf.begin(), buf.begin() + len);
		}
		applicator.setTextDelimiter(rx);
	}
	if (options[DEP_DELIMIT].doesOccur) {
		if (!options[DEP_DELIMIT].value.empty()) {
			applicator.dep_delimit = std::stoul(options[DEP_DELIMIT].value);
		}
		else {
			applicator.dep_delimit = 10;
		}
		applicator.parse_dep = true;
	}
	if (options[DEP_ABSOLUTE].doesOccur) {
		applicator.dep_absolute = true;
	}
	if (options[DEP_ORIGINAL].doesOccur) {
		applicator.dep_original = true;
	}
	if (options[DEP_ALLOW_LOOPS].doesOccur) {
		applicator.dep_block_loops = false;
	}
	if (options[DEP_BLOCK_CROSSING].doesOccur) {
		applicator.dep_block_crossing = true;
	}
	if (options[MAGIC_READINGS].doesOccur) {
		applicator.allow_magic_readings = false;
	}
	if (options[NO_PASS_ORIGIN].doesOccur) {
		applicator.no_pass_origin = true;
	}
	if (options[SPLIT_MAPPINGS].doesOccur) {
		applicator.split_mappings = true;
	}
	if (options[SHOW_END_TAGS].doesOccur) {
		applicator.show_end_tags = true;
	}
	if (options[NO_BREAK].doesOccur) {
		applicator.add_spacing = false;
	}
}
