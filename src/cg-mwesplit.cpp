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
#include "MweSplitApplicator.hpp"

#include "version.hpp"

using CG3::CG3Quit;

#include <uoptions.h>
namespace Options {
enum OPTIONS {
	HELP1,
	HELP2,
	NUM_OPTIONS,
};
UOption options[] = {
	UOPTION_DEF_D("help", 'h', UOPT_NO_ARG,       "shows this help"),
	UOPTION_DEF_D("?",    '?', UOPT_NO_ARG,       "shows this help"),
};
}
using namespace Options;

int main(int argc, char** argv) {
	UErrorCode status = U_ZERO_ERROR;

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}

	U_MAIN_INIT_ARGS(argc, argv);
	argc = u_parseArgs(argc, argv, NUM_OPTIONS, options);

	if (argc < 0 || options[HELP1].doesOccur || options[HELP2].doesOccur) {
		FILE* out = (argc < 0) ? stderr : stdout;
		fprintf(out, "Usage: cg-mwesplit [OPTIONS]\n");
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

	ucnv_setDefaultName("UTF-8");
	uloc_setDefault("en_US_POSIX", &status);

	CG3::MweSplitApplicator applicator(std::cerr);

	applicator.verbosity_level = 0;
	applicator.runGrammarOnText(std::cin, std::cout);

	u_cleanup();
}
