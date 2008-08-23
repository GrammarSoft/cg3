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
		VERBOSE,
		VISLCGCOMPAT,
		STDIN,
		STDOUT,
		STDERR,
		CODEPAGE_GLOBAL,
		CODEPAGE_GRAMMAR,
		CODEPAGE_INPUT,
		CODEPAGE_OUTPUT,
		LOCALE_GLOBAL,
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
		NO_PASS_ORIGIN,
		NUM_OPTIONS
	};

	UOption options[]= {
		UOPTION_DEF_D("help",				'h', UOPT_NO_ARG, "shows this help"),
		UOPTION_DEF_D("?",					'?', UOPT_NO_ARG, "shows this help"),
		UOPTION_DEF_D("version",			'V', UOPT_NO_ARG, "prints copyright and version information"),
		UOPTION_DEF_D("grammar",			'g', UOPT_REQUIRES_ARG, "specifies the grammar file to use for disambiguation"),
		UOPTION_DEF_D("grammar-out",		0, UOPT_REQUIRES_ARG, "writes the compiled grammar in textual form to a file"),
		UOPTION_DEF_D("grammar-bin",		0, UOPT_REQUIRES_ARG, "writes the compiled grammar in binary form to a file"),
		UOPTION_DEF_D("grammar-info",		0, UOPT_REQUIRES_ARG, "writes the compiled grammar in textual form to a file, with lots of statistics and information"),
		UOPTION_DEF_D("grammar-only",		0, UOPT_NO_ARG, "only compiles the grammar; implies --verbose"),
		UOPTION_DEF_D("check-only",			0, UOPT_NO_ARG, "same as --grammar-only"),
		UOPTION_DEF("unsafe",				'u', UOPT_NO_ARG),
		UOPTION_DEF_D("sections",			's', UOPT_REQUIRES_ARG, "number of sections to run; defaults to all sections"),
		UOPTION_DEF("debug",				'd', UOPT_OPTIONAL_ARG),
		UOPTION_DEF_D("verbose",			'v', UOPT_OPTIONAL_ARG, "increases verbosity"),
		UOPTION_DEF("vislcg-compat",		'P', UOPT_NO_ARG),

		UOPTION_DEF_D("stdin",				'I', UOPT_REQUIRES_ARG, "file to print output to instead of stdout"),
		UOPTION_DEF_D("stdout",				'O', UOPT_REQUIRES_ARG, "file to read input from instead of stdin"),
		UOPTION_DEF_D("stderr",				'E', UOPT_REQUIRES_ARG, "file to print errors to instead of stderr"),

		UOPTION_DEF_D("codepage-all",		'C', UOPT_REQUIRES_ARG, "codepage to use for grammar, input, and output streams; defaults to environment settings"),
		UOPTION_DEF_D("codepage-grammar",	0, UOPT_REQUIRES_ARG, "codepage to use for grammar; overrides --codepage-all"),
		UOPTION_DEF_D("codepage-input",		0, UOPT_REQUIRES_ARG, "codepage to use for input; overrides --codepage-all"),
		UOPTION_DEF_D("codepage-output",	0, UOPT_REQUIRES_ARG, "codepage to use for output and errors; overrides --codepage-all"),

		UOPTION_DEF_D("locale-all",			'L', UOPT_REQUIRES_ARG, "locale to use for grammar, input, and output streams; defaults to en_US_POSIX"),
		UOPTION_DEF_D("locale-grammar",		0, UOPT_REQUIRES_ARG, "locale to use for grammar; overrides --locale-all"),
		UOPTION_DEF_D("locale-input",		0, UOPT_REQUIRES_ARG, "locale to use for input; overrides --locale-all"),
		UOPTION_DEF_D("locale-output",		0, UOPT_REQUIRES_ARG, "locale to use for output and errors; overrides --locale-all"),

		UOPTION_DEF_D("no-mappings",		0, UOPT_NO_ARG, "disables all MAP, ADD, and REPLACE rules"),
		UOPTION_DEF_D("no-corrections",		0, UOPT_NO_ARG, "disables all SUBSTITUTE and APPEND rules"),
		UOPTION_DEF_D("no-before-sections",	0, UOPT_NO_ARG, "disables all rules in BEFORE-SECTIONS parts"),
		UOPTION_DEF_D("no-sections",		0, UOPT_NO_ARG, "disables all rules in SECTION parts"),
		UOPTION_DEF_D("no-after-sections",	0, UOPT_NO_ARG, "disables all rules in AFTER-SECTIONS parts"),

		UOPTION_DEF_D("trace",				't', UOPT_NO_ARG, "prints debug output alongside with normal output"),
		UOPTION_DEF_D("trace-name-only",	0, UOPT_NO_ARG, "if a rule is named, omit the line number; implies --trace"),
		UOPTION_DEF_D("trace-no-removed",	0, UOPT_NO_ARG, "does not print removed readings; implies --trace"),
		UOPTION_DEF_D("single-run",			0, UOPT_NO_ARG, "runs each section only once"),
		UOPTION_DEF("statistics",			'S', UOPT_NO_ARG),
		UOPTION_DEF_D("prefix",			    'p', UOPT_REQUIRES_ARG, "sets the mapping prefix; defaults to @"),

		UOPTION_DEF_D("num-windows",		0, UOPT_REQUIRES_ARG, "number of windows to keep in before/ahead buffers; defaults to 2"),
		UOPTION_DEF_D("always-span",		0, UOPT_NO_ARG, "forces scanning tests to always span across window boundaries"),
		UOPTION_DEF_D("soft-limit",			0, UOPT_REQUIRES_ARG, "number of cohorts after which the SOFT-DELIMITERS kick in; defaults to 300"),
		UOPTION_DEF_D("hard-limit",			0, UOPT_REQUIRES_ARG, "number of cohorts after which the window is forcefully cut; defaults to 500"),
		UOPTION_DEF("dep-delimit",			0, UOPT_NO_ARG),
		UOPTION_DEF("dep-reenum",			0, UOPT_NO_ARG),
		UOPTION_DEF("dep-humanize",			0, UOPT_NO_ARG),
		UOPTION_DEF("dep-original",			0, UOPT_NO_ARG),
		UOPTION_DEF_D("dep-allow-loops",	0, UOPT_NO_ARG, "allows the creation of circular dependencies"),

		UOPTION_DEF_D("no-magic-readings",	0, UOPT_NO_ARG, "prevents running rules on magic readings"),
		UOPTION_DEF_D("no-pass-origin",		'o', UOPT_NO_ARG, "prevents scanning tests from passing the point of origin")
	};
}

#endif
