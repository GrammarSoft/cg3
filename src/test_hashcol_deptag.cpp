/*
* Copyright (C) 2007, GrammarSoft ApS
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "icu_uoptions.h"
#include "Tag.h"

using namespace CG3;

UFILE *ux_stdin = 0;
UFILE *ux_stdout = 0;
UFILE *ux_stderr = 0;

int main(int argc, char* argv[]) {
	UErrorCode status = U_ZERO_ERROR;
	srand((uint32_t)time(0));

	U_MAIN_INIT_ARGS(argc, argv);

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}
	status = U_ZERO_ERROR;

	ucnv_setDefaultName("UTF-8");
	const char *codepage_default = ucnv_getDefaultName();
	const char *codepage_grammar = codepage_default;
	const char *codepage_input   = codepage_grammar;
	const char *codepage_output  = codepage_grammar;

	uloc_setDefault("en_US_POSIX", &status);
	const char *locale_default = uloc_getDefault();
	const char *locale_grammar = locale_default;
	const char *locale_input   = locale_grammar;
	const char *locale_output  = locale_grammar;

	ux_stdout = u_finit(stdout, locale_output, codepage_output);
	if (!ux_stdout) {
		std::cerr << "Error: Failed to open the output stream for writing!" << std::endl;
		CG3Quit(1);
	}

	ux_stderr = u_finit(stderr, locale_output, codepage_output);
	if (!ux_stdout) {
		std::cerr << "Error: Failed to open the error stream for writing!" << std::endl;
		CG3Quit(1);
	}

	ux_stdin = u_finit(stdin, locale_input, codepage_input);
	if (!ux_stdin) {
		std::cerr << "Error: Failed to open the input stream for reading!" << std::endl;
		CG3Quit(1);
	}

	init_gbuffers();
	init_strings();
	init_keywords();
	init_flags();

	UChar *t;
	stdext::hash_map<uint32_t, Tag*> m;

	t = new UChar[512];
	memset(t, 0, sizeof(UChar)*512);

	for (uint32_t i=0 ; i<1000 ; i++) {
		for (uint32_t j=0 ; j<1000 ; j++) {
			memset(t, 0, sizeof(UChar)*512);
			u_sprintf(t, "#%u->%u", i, j);
			
			Tag *x = new Tag();
			x->parseTag(t, ux_stderr);
			uint32_t h = x->rehash();

			if (m.find(h) != m.end()) {
				const Tag *o = m.find(h)->second;
				if (u_strcmp(x->tag, o->tag) != 0) {
					u_fprintf(ux_stdout, "Collision value %u between '%S' and '%S'\n", h, x->tag, o->tag);
					u_fflush(ux_stdout);
				}
				delete x;
			}
			else {
				m[h] = x;
			}
		}
	}

	u_fclose(ux_stdin);
	u_fclose(ux_stdout);
	u_fclose(ux_stderr);
	
	free_strings();
	free_keywords();
	free_gbuffers();
	free_flags();

	u_cleanup();

	return status;
}
