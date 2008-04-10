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

#ifndef __OPTIONS_H
#define __OPTIONS_H

namespace Options {
	enum OPTIONS {
		HELP1,
		HELP2,
		VERSION,
		GRAMMAR,
		GRAMMAR_OUT,
		GRAMMAR_BIN,
		GRAMMAR_INFO,
		GRAMMAR_ONLY,
		CHECK_ONLY,
		UNSAFE,
		SECTIONS,
		DODEBUG,
		VISLCGCOMPAT,
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
		NOMAPPINGS,
		NOCORRECTIONS,
		NOBEFORESECTIONS,
		NOSECTIONS,
		NOAFTERSECTIONS,
		TRACE,
		TRACE_NAME_ONLY,
		TRACE_NO_REMOVED,
		SINGLERUN,
		STATISTICS,
		MAPPING_PREFIX,
		NUM_WINDOWS,
		ALWAYS_SPAN,
		SOFT_LIMIT,
		HARD_LIMIT,
		DEP_DELIMIT,
		DEP_REENUM,
		DEP_HUMANIZE,
		DEP_ORIGINAL,
		DEP_ALLOW_LOOPS,
		MAGIC_READINGS,
		NUM_OPTIONS
	};

	UOption options[]= {
		UOPTION_DEF("help",					'h', UOPT_NO_ARG),
		UOPTION_DEF("?",					'?', UOPT_NO_ARG),
		UOPTION_DEF("version",				'V', UOPT_NO_ARG),
		UOPTION_DEF("grammar",				'g', UOPT_REQUIRES_ARG),
		UOPTION_DEF("grammar-out",			0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("grammar-bin",			0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("grammar-info",			0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("grammar-only",			0, UOPT_NO_ARG),
		UOPTION_DEF("check-only",			0, UOPT_NO_ARG),
		UOPTION_DEF("unsafe",				'u', UOPT_NO_ARG),
		UOPTION_DEF("sections",				's', UOPT_REQUIRES_ARG),
		UOPTION_DEF("debug",				'd', UOPT_OPTIONAL_ARG),
		UOPTION_DEF("vislcg-compat",		'p', UOPT_NO_ARG),

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
		UOPTION_DEF("locale-output",		0, UOPT_REQUIRES_ARG),

		UOPTION_DEF("no-mappings",			0, UOPT_NO_ARG),
		UOPTION_DEF("no-corrections",		0, UOPT_NO_ARG),
		UOPTION_DEF("no-before-sections",	0, UOPT_NO_ARG),
		UOPTION_DEF("no-sections",			0, UOPT_NO_ARG),
		UOPTION_DEF("no-after-sections",	0, UOPT_NO_ARG),

		UOPTION_DEF("trace",				0, UOPT_NO_ARG),
		UOPTION_DEF("trace-name-only",		0, UOPT_NO_ARG),
		UOPTION_DEF("trace-no-removed",		0, UOPT_NO_ARG),
		UOPTION_DEF("single-run",			0, UOPT_NO_ARG),
		UOPTION_DEF("statistics",			'S', UOPT_NO_ARG),
		UOPTION_DEF("prefix",			    0, UOPT_REQUIRES_ARG),

		UOPTION_DEF("num-windows",			0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("always-span",			0, UOPT_NO_ARG),
		UOPTION_DEF("soft-limit",			0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("hard-limit",			0, UOPT_REQUIRES_ARG),
		UOPTION_DEF("dep-delimit",			0, UOPT_NO_ARG),
		UOPTION_DEF("dep-reenum",			0, UOPT_NO_ARG),
		UOPTION_DEF("dep-humanize",			0, UOPT_NO_ARG),
		UOPTION_DEF("dep-original",			0, UOPT_NO_ARG),
		UOPTION_DEF("dep-allow-loops",		0, UOPT_NO_ARG),

		UOPTION_DEF("no-magic-readings",	0, UOPT_NO_ARG)
	};
}

#endif
