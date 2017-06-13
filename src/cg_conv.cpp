/*
* Copyright (C) 2007-2017, GrammarSoft ApS
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
#include "FormatConverter.hpp"

#include "version.hpp"

#include "options_conv.hpp"
using namespace Options;
using namespace std;
using CG3::CG3Quit;

int main(int argc, char *argv[]) {
	UErrorCode status = U_ZERO_ERROR;
	UFILE *ux_stdin = 0;
	UFILE *ux_stdout = 0;
	UFILE *ux_stderr = 0;

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}

	U_MAIN_INIT_ARGS(argc, argv);
	argc = u_parseArgs(argc, argv, NUM_OPTIONS, options);

	if (argc < 0 || options[HELP1].doesOccur || options[HELP2].doesOccur) {
		FILE *out = (argc < 0) ? stderr : stdout;
		fprintf(out, "Usage: cg-conv [OPTIONS]\n");
		fprintf(out, "\n");
		fprintf(out, "Options:\n");

		size_t longest = 0;
		for (uint32_t i = 0; i < NUM_OPTIONS; i++) {
			if (options[i].description) {
				size_t len = strlen(options[i].longName);
				longest = std::max(longest, len);
			}
		}
		for (uint32_t i = 0; i < NUM_OPTIONS; i++) {
			if (options[i].description && options[i].description[0] != '!') {
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
				fprintf(out, "  %s", options[i].description);
				fprintf(out, "\n");
			}
		}

		return argc < 0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
	}

	if (options[IN_CG2].doesOccur) {
		options[IN_CG].doesOccur = true;
	}
	if (options[OUT_CG2].doesOccur) {
		options[OUT_CG].doesOccur = true;
	}

	ucnv_setDefaultName("UTF-8");
	const char *codepage_default = ucnv_getDefaultName();
	uloc_setDefault("en_US_POSIX", &status);
	const char *locale_default = uloc_getDefault();

	ux_stdin = u_finit(stdin, locale_default, codepage_default);
	ux_stdout = u_finit(stdout, locale_default, codepage_default);
	ux_stderr = u_finit(stderr, locale_default, codepage_default);

	CG3::Grammar grammar;

	grammar.ux_stderr = ux_stderr;
	grammar.allocateDummySet();
	grammar.delimiters = grammar.allocateSet();
	grammar.addTagToSet(grammar.allocateTag(CG3::stringbits[0].getTerminatedBuffer()), grammar.delimiters);
	grammar.reindex();

	CG3::FormatConverter applicator(ux_stderr);
	applicator.setGrammar(&grammar);

	std::unique_ptr<CG3::istream> instream;

	CG3::CG_FORMATS fmt = CG3::FMT_INVALID;

	if (options[ADD_TAGS].doesOccur) {
		options[IN_PLAIN].doesOccur = true;
		dynamic_cast<CG3::PlaintextApplicator&>(applicator).add_tags = true;
	}

	if (options[IN_CG].doesOccur) {
		fmt = CG3::FMT_CG;
	}
	else if (options[IN_NICELINE].doesOccur) {
		fmt = CG3::FMT_NICELINE;
	}
	else if (options[IN_APERTIUM].doesOccur) {
		fmt = CG3::FMT_APERTIUM;
	}
	else if (options[IN_FST].doesOccur) {
		fmt = CG3::FMT_FST;
	}
	else if (options[IN_PLAIN].doesOccur) {
		fmt = CG3::FMT_PLAIN;
	}

	if (options[IN_AUTO].doesOccur || fmt == CG3::FMT_INVALID) {
		CG3::UString buffer;
		buffer.resize(1000);
		int32_t nr = u_file_read(&buffer[0], buffer.size(), ux_stdin);
		buffer.resize(nr);
		URegularExpression *rx = 0;

		for (;;) {
			rx = uregex_openC("^\"<[^>]+>\".*?^\\s+\"[^\"]+\"", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
			uregex_setText(rx, buffer.c_str(), buffer.size(), &status);
			if (uregex_find(rx, -1, &status)) {
				fmt = CG3::FMT_CG;
				break;
			}
			uregex_close(rx);

			rx = uregex_openC("^\\S+ *\t *\\[\\S+\\]", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
			uregex_setText(rx, buffer.c_str(), buffer.size(), &status);
			if (uregex_find(rx, -1, &status)) {
				fmt = CG3::FMT_NICELINE;
				break;
			}
			uregex_close(rx);

			rx = uregex_openC("^\\S+ *\t *\"\\S+\"", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
			uregex_setText(rx, buffer.c_str(), buffer.size(), &status);
			if (uregex_find(rx, -1, &status)) {
				fmt = CG3::FMT_NICELINE;
				break;
			}
			uregex_close(rx);

			rx = uregex_openC("\\^[^/]+(/[^<]+(<[^>]+>)+)+\\$", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
			uregex_setText(rx, buffer.c_str(), buffer.size(), &status);
			if (uregex_find(rx, -1, &status)) {
				fmt = CG3::FMT_APERTIUM;
				break;
			}
			uregex_close(rx);

			rx = uregex_openC("^\\S+\t\\S+(\\+\\S+)+$", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
			uregex_setText(rx, buffer.c_str(), buffer.size(), &status);
			if (uregex_find(rx, -1, &status)) {
				fmt = CG3::FMT_FST;
				break;
			}

			fmt = CG3::FMT_PLAIN;
			break;
		}
		uregex_close(rx);

		instream.reset(new CG3::istream_buffer(ux_stdin, buffer));
	}
	else {
		instream.reset(new CG3::istream(ux_stdin));
	}

	applicator.setInputFormat(fmt);

	if (options[SUB_LTR].doesOccur) {
		grammar.sub_readings_ltr = true;
	}
	if (options[MAPPING_PREFIX].doesOccur) {
		size_t sn = strlen(options[MAPPING_PREFIX].value);
		CG3::UString buf(sn * 3, 0);
		UConverter *conv = ucnv_open(codepage_default, &status);
		ucnv_toUChars(conv, &buf[0], buf.size(), options[MAPPING_PREFIX].value, sn, &status);
		ucnv_close(conv);
		grammar.mapping_prefix = buf[0];
	}
	if (options[SUB_DELIMITER].doesOccur) {
		size_t sn = strlen(options[SUB_DELIMITER].value);
		applicator.sub_delims.resize(sn * 2);
		UConverter *conv = ucnv_open(codepage_default, &status);
		sn = ucnv_toUChars(conv, &applicator.sub_delims[0], applicator.sub_delims.size(), options[SUB_DELIMITER].value, sn, &status);
		applicator.sub_delims.resize(sn);
		applicator.sub_delims += '+';
		ucnv_close(conv);
	}
	if (options[FST_WTAG].doesOccur) {
		size_t sn = strlen(options[FST_WTAG].value);
		applicator.wtag.resize(sn * 2);
		UConverter *conv = ucnv_open(codepage_default, &status);
		sn = ucnv_toUChars(conv, &applicator.wtag[0], applicator.wtag.size(), options[FST_WTAG].value, sn, &status);
		applicator.wtag.resize(sn);
		ucnv_close(conv);
	}
	if (options[FST_WFACTOR].doesOccur) {
		applicator.wfactor = strtof(options[FST_WFACTOR].value, 0);
	}

	applicator.setOutputFormat(CG3::FMT_CG);

	if (options[OUT_APERTIUM].doesOccur) {
		applicator.setOutputFormat(CG3::FMT_APERTIUM);
	}
	else if (options[OUT_NICELINE].doesOccur) {
		applicator.setOutputFormat(CG3::FMT_NICELINE);
	}
	else if (options[OUT_PLAIN].doesOccur) {
		applicator.setOutputFormat(CG3::FMT_PLAIN);
	}

	applicator.is_conv = true;
	applicator.verbosity_level = 0;
	applicator.runGrammarOnText(*instream.get(), ux_stdout);

	u_fclose(ux_stdout);
	u_fclose(ux_stderr);

	u_cleanup();
}
