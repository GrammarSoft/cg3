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
#include "FormatConverter.h"

#include "version.h"

using namespace std;
using CG3::CG3Quit;

int main(int argc, char *argv[]) {
	UErrorCode status = U_ZERO_ERROR;
	UFILE *ux_stdin = 0;
	UFILE *ux_stdout = 0;
	UFILE *ux_stderr = 0;

	argc=argc;
	argv=argv;

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}

	const char *codepage_default = ucnv_getDefaultName();
	ucnv_setDefaultName("UTF-8");
	const char *locale_default = "en_US_POSIX"; //uloc_getDefault();
	uloc_setDefault("en_US_POSIX", &status);

	std::cerr << "Codepage " << codepage_default << ", locale " << locale_default << std::endl;

	ux_stdin = u_finit(stdin, locale_default, codepage_default);
	ux_stdout = u_finit(stdout, locale_default, codepage_default);
	ux_stderr = u_finit(stderr, locale_default, codepage_default);

	CG3::Grammar grammar;

	grammar.ux_stderr = ux_stderr;
	grammar.reindex();

	CG3::FormatConverter applicator(ux_stderr);
	applicator.setGrammar(&grammar);
	applicator.setInputFormat(CG3::FMT_VISL);
	applicator.setOutputFormat(CG3::FMT_APERTIUM);
	applicator.verbosity_level = 0;
	applicator.runGrammarOnText(ux_stdin, ux_stdout);

	u_fclose(ux_stdin);
	u_fclose(ux_stdout);
	u_fclose(ux_stderr);

	u_cleanup();

	return EXIT_SUCCESS;
}
