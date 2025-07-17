/*
* Copyright (C) 2007-2025, GrammarSoft ApS
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.	If not, see <https://www.gnu.org/licenses/>.
*/

#include "stdafx.hpp"
#include "Grammar.hpp"
#include "FormatConverter.hpp"

#include "version.hpp"

#include "options_conv.hpp"
#include "options_parser.hpp"
using namespace Options;
using namespace OptionsConv;
using namespace CG3;

int main(int argc, char* argv[]) {
	UErrorCode status = U_ZERO_ERROR;

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}

	argc = u_parseArgs(argc, argv, options_conv.size(), options_conv.data());

	parse_opts_env("CG3_CONV_DEFAULT", options_default);
	parse_opts_env("CG3_CONV_OVERRIDE", options_override);
	for (size_t i = 0; i < options_conv.size(); ++i) {
		if (options_default[i].doesOccur && !options_conv[i].doesOccur) {
			options_conv[i] = options_default[i];
		}
		if (options_override[i].doesOccur) {
			options_conv[i] = options_override[i];
		}
	}

	if (argc < 0 || options_conv[HELP1].doesOccur || options_conv[HELP2].doesOccur) {
		FILE* out = (argc < 0) ? stderr : stdout;
		fprintf(out, "Usage: cg-conv [OPTIONS]\n");
		fprintf(out, "\n");
		fprintf(out, "Environment variable:\n");
		fprintf(out, " CG3_CONV_DEFAULT: Sets default cmdline options_conv, which the actual passed options_conv will override.\n");
		fprintf(out, " CG3_CONV_OVERRIDE: Sets forced cmdline options_conv, which will override any passed option.\n");
		fprintf(out, "\n");
		fprintf(out, "Options:\n");

		size_t longest = 0;
		for (uint32_t i = 0; i < options_conv.size(); i++) {
			if (!options_conv[i].description.empty()) {
				size_t len = strlen(options_conv[i].longName);
				longest = std::max(longest, len);
			}
		}
		for (uint32_t i = 0; i < options_conv.size(); i++) {
			if (!options_conv[i].description.empty() && options_conv[i].description[0] != '!') {
				fprintf(out, " ");
				if (options_conv[i].shortName) {
					fprintf(out, "-%c,", options_conv[i].shortName);
				}
				else {
					fprintf(out, "   ");
				}
				fprintf(out, " --%s", options_conv[i].longName);
				size_t ldiff = longest - strlen(options_conv[i].longName);
				while (ldiff--) {
					fprintf(out, " ");
				}
				fprintf(out, "  %s", options_conv[i].description.c_str());
				fprintf(out, "\n");
			}
		}

		fprintf(out, "\n\nKeys for JSONL format:\n");
		fprintf(out, "===============================================================================\n");
		fprintf(out, "Cohort:                     Reading:                   Stream Command:\n");
		fprintf(out, "    w  wordform/token          l  lemma/base form        cmd  stream command\n");
		fprintf(out, "  sts  static tags            ts  tags\n");
		fprintf(out, "   rs  readings                s  subreading\n");
		fprintf(out, "  drs  deleted readings                                Plain text:\n");
		fprintf(out, "   ds  dependency self                                     t  text line\n");
		fprintf(out, "   dp  dependency parent\n");
		fprintf(out, "    z  text line(s) suffix\n");
		fprintf(out, "===============================================================================\n");

		return argc < 0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
	}

	if (options_conv[IN_CG2].doesOccur) {
		options_conv[IN_CG].doesOccur = true;
	}
	if (options_conv[OUT_CG2].doesOccur) {
		options_conv[OUT_CG].doesOccur = true;
	}

	ucnv_setDefaultName("UTF-8");
	const char* codepage_default = ucnv_getDefaultName();
	uloc_setDefault("en_US_POSIX", &status);

	FormatConverter applicator(std::cerr);

	Grammar& grammar = applicator.conv_grammar;
	if (options_conv[ORDERED].doesOccur) {
		grammar.ordered = true;
	}

	ux_stripBOM(std::cin);

	std::istream* instream = &std::cin;
	std::unique_ptr<std::istream> _instream;

	cg3_sformat fmt = CG3SF_INVALID;

	if (options_conv[ADD_TAGS].doesOccur) {
		options_conv[IN_PLAIN].doesOccur = true;
		dynamic_cast<PlaintextApplicator&>(applicator).add_tags = true;
	}

	if (options_conv[IN_CG].doesOccur) {
		fmt = CG3SF_CG;
	}
	else if (options_conv[IN_NICELINE].doesOccur) {
		fmt = CG3SF_NICELINE;
	}
	else if (options_conv[IN_APERTIUM].doesOccur) {
		fmt = CG3SF_APERTIUM;
	}
	else if (options_conv[IN_FST].doesOccur) {
		fmt = CG3SF_FST;
	}
	else if (options_conv[IN_PLAIN].doesOccur) {
		fmt = CG3SF_PLAIN;
	}
	else if (options_conv[IN_JSONL].doesOccur) {
		fmt = CG3SF_JSONL;
	}
	else if (options_conv[IN_BINARY].doesOccur) {
		fmt = CG3SF_BINARY;
	}

	if (options_conv[IN_AUTO].doesOccur || fmt == CG3SF_INVALID) {
		_instream = applicator.detectFormat(std::cin);
		instream = _instream.get();
		fmt = applicator.fmt_input;
	}

	applicator.fmt_input = fmt;

	if (options_conv[SUB_LTR].doesOccur) {
		grammar.sub_readings_ltr = true;
	}
	if (options_conv[MAPPING_PREFIX].doesOccur) {
		auto sn = SI32(options_conv[MAPPING_PREFIX].value.size());
		UString buf(sn * 3, 0);
		UConverter* conv = ucnv_open(codepage_default, &status);
		ucnv_toUChars(conv, &buf[0], SI32(buf.size()), options_conv[MAPPING_PREFIX].value.c_str(), sn, &status);
		ucnv_close(conv);
		grammar.mapping_prefix = buf[0];
	}
	if (options_conv[SUB_DELIMITER].doesOccur) {
		auto sn = SI32(options_conv[SUB_DELIMITER].value.size());
		applicator.sub_delims.resize(sn * 2);
		UConverter* conv = ucnv_open(codepage_default, &status);
		sn = ucnv_toUChars(conv, &applicator.sub_delims[0], SI32(applicator.sub_delims.size()), options_conv[SUB_DELIMITER].value.c_str(), sn, &status);
		applicator.sub_delims.resize(sn);
		applicator.sub_delims += '+';
		ucnv_close(conv);
	}
	if (options_conv[FST_WTAG].doesOccur) {
		auto sn = SI32(options_conv[FST_WTAG].value.size());
		applicator.wtag.resize(sn * 2);
		UConverter* conv = ucnv_open(codepage_default, &status);
		sn = ucnv_toUChars(conv, &applicator.wtag[0], SI32(applicator.wtag.size()), options_conv[FST_WTAG].value.c_str(), sn, &status);
		applicator.wtag.resize(sn);
		ucnv_close(conv);
	}
	if (options_conv[FST_WFACTOR].doesOccur) {
		applicator.wfactor = std::stod(options_conv[FST_WFACTOR].value);
	}

	applicator.fmt_output = CG3SF_CG;

	if (options_conv[OUT_APERTIUM].doesOccur) {
		applicator.fmt_output = CG3SF_APERTIUM;
		applicator.unicode_tags = true;
	}
	else if (options_conv[OUT_FST].doesOccur) {
		applicator.fmt_output = CG3SF_FST;
	}
	else if (options_conv[OUT_NICELINE].doesOccur) {
		applicator.fmt_output = CG3SF_NICELINE;
	}
	else if (options_conv[OUT_PLAIN].doesOccur) {
		applicator.fmt_output = CG3SF_PLAIN;
	}
	else if (options_conv[OUT_JSONL].doesOccur) {
		applicator.fmt_output = CG3SF_JSONL;
	}
	else if (options_conv[OUT_BINARY].doesOccur) {
		applicator.fmt_output = CG3SF_BINARY;
	}

	if (options_conv[UNICODE_TAGS].doesOccur) {
		applicator.unicode_tags = true;
	}
	if (options_conv[PIPE_DELETED].doesOccur) {
		applicator.pipe_deleted = true;
	}
	if (options_conv[NO_BREAK].doesOccur) {
		applicator.add_spacing = false;
	}


	if (options_conv[PARSE_DEP].doesOccur) {
		applicator.parse_dep = true;
		applicator.has_dep = true;
	}
	if (options_conv[DEP_DELIMIT].doesOccur) {
		if (!options_conv[DEP_DELIMIT].value.empty()) {
			applicator.dep_delimit = std::stoul(options_conv[DEP_DELIMIT].value);
		}
		else {
			applicator.dep_delimit = 10;
		}
		applicator.parse_dep = true;
	}
	applicator.is_conv = true;
	applicator.trace = true;
	applicator.verbosity_level = 0;
	applicator.runGrammarOnText(*instream, std::cout);

	u_cleanup();
}
