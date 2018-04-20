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

#pragma once
#ifndef c6d28b7452ec699b_OPTIONS_H
#define c6d28b7452ec699b_OPTIONS_H

#include <uoptions.h>

namespace Options {
enum OPTIONS {
	HELP1,
	HELP2,
	VERSION,
	VERSION_TOO_OLD,
	GRAMMAR,
	GRAMMAR_OUT,
	GRAMMAR_BIN,
	GRAMMAR_ONLY,
	ORDERED,
	UNSAFE,
	SECTIONS,
	RULES,
	RULE,
	DODEBUG,
	VERBOSE,
	QUIET,
	VISLCGCOMPAT,
	STDIN,
	STDOUT,
	STDERR,
	CODEPAGE_GLOBAL,
	CODEPAGE_GRAMMAR,
	CODEPAGE_INPUT,
	CODEPAGE_OUTPUT,
	NOMAPPINGS,
	NOCORRECTIONS,
	NOBEFORESECTIONS,
	NOSECTIONS,
	NOAFTERSECTIONS,
	TRACE,
	TRACE_NAME_ONLY,
	TRACE_NO_REMOVED,
	TRACE_ENCL,
	DRYRUN,
	SINGLERUN,
	MAXRUNS,
	STATISTICS,
	OPTIMIZE_UNSAFE,
	OPTIMIZE_SAFE,
	MAPPING_PREFIX,
	UNICODE_TAGS,
	UNIQUE_TAGS,
	NUM_WINDOWS,
	ALWAYS_SPAN,
	SOFT_LIMIT,
	HARD_LIMIT,
	DEP_DELIMIT,
	DEP_ORIGINAL,
	DEP_ALLOW_LOOPS,
	DEP_BLOCK_CROSSING,
	MAGIC_READINGS,
	NO_PASS_ORIGIN,
	SPLIT_MAPPINGS,
	SHOW_END_TAGS,
	SHOW_UNUSED_SETS,
	SHOW_TAGS,
	SHOW_TAG_HASHES,
	SHOW_SET_HASHES,
	DUMP_AST,
	NUM_OPTIONS,
};

UOption options[] = {
	UOPTION_DEF_D("help",                'h', UOPT_NO_ARG,       "shows this help"),
	UOPTION_DEF_D("?",                   '?', UOPT_NO_ARG,       "shows this help"),
	UOPTION_DEF_D("version",             'V', UOPT_NO_ARG,       "prints copyright and version information"),
	UOPTION_DEF_D("min-binary-revision",   0, UOPT_NO_ARG,       "prints the minimum usable binary grammar revision"),
	UOPTION_DEF_D("grammar",             'g', UOPT_REQUIRES_ARG, "specifies the grammar file to use for disambiguation"),
	UOPTION_DEF_D("grammar-out",           0, UOPT_REQUIRES_ARG, "writes the compiled grammar in textual form to a file"),
	UOPTION_DEF_D("grammar-bin",           0, UOPT_REQUIRES_ARG, "writes the compiled grammar in binary form to a file"),
	UOPTION_DEF_D("grammar-only",          0, UOPT_NO_ARG,       "only compiles the grammar; implies --verbose"),
	UOPTION_DEF_D("ordered",               0, UOPT_NO_ARG,       "(will in future allow full ordered matching)"),
	UOPTION_DEF_D("unsafe",              'u', UOPT_NO_ARG,       "allows the removal of all readings in a cohort, even the last one"),
	UOPTION_DEF_D("sections",            's', UOPT_REQUIRES_ARG, "number or ranges of sections to run; defaults to all sections"),
	UOPTION_DEF_D("rules",                 0, UOPT_REQUIRES_ARG, "number or ranges of rules to run; defaults to all rules"),
	UOPTION_DEF_D("rule",                  0, UOPT_REQUIRES_ARG, "a name or number of a single rule to run"),
	UOPTION_DEF_D("debug",               'd', UOPT_OPTIONAL_ARG, "enables debug output (very noisy)"),
	UOPTION_DEF_D("verbose",             'v', UOPT_OPTIONAL_ARG, "increases verbosity"),
	UOPTION_DEF_D("quiet",                 0, UOPT_NO_ARG,       "squelches warnings (same as -v 0)"),
	UOPTION_DEF_D("vislcg-compat",       '2', UOPT_NO_ARG,       "enables compatibility mode for older CG-2 and vislcg grammars"),

	UOPTION_DEF_D("stdin",               'I', UOPT_REQUIRES_ARG, "file to read input from instead of stdin"),
	UOPTION_DEF_D("stdout",              'O', UOPT_REQUIRES_ARG, "file to print output to instead of stdout"),
	UOPTION_DEF_D("stderr",              'E', UOPT_REQUIRES_ARG, "file to print errors to instead of stderr"),

	UOPTION_DEF_D("codepage-all",        'C', UOPT_REQUIRES_ARG, nullptr),
	UOPTION_DEF_D("codepage-grammar",      0, UOPT_REQUIRES_ARG, nullptr),
	UOPTION_DEF_D("codepage-input",        0, UOPT_REQUIRES_ARG, nullptr),
	UOPTION_DEF_D("codepage-output",       0, UOPT_REQUIRES_ARG, nullptr),

	UOPTION_DEF_D("no-mappings",           0, UOPT_NO_ARG,       "disables all MAP, ADD, and REPLACE rules"),
	UOPTION_DEF_D("no-corrections",        0, UOPT_NO_ARG,       "disables all SUBSTITUTE and APPEND rules"),
	UOPTION_DEF_D("no-before-sections",    0, UOPT_NO_ARG,       "disables all rules in BEFORE-SECTIONS parts"),
	UOPTION_DEF_D("no-sections",           0, UOPT_NO_ARG,       "disables all rules in SECTION parts"),
	UOPTION_DEF_D("no-after-sections",     0, UOPT_NO_ARG,       "disables all rules in AFTER-SECTIONS parts"),

	UOPTION_DEF_D("trace",               't', UOPT_OPTIONAL_ARG, "prints debug output alongside normal output; optionally stops execution"),
	UOPTION_DEF_D("trace-name-only",       0, UOPT_NO_ARG,       "if a rule is named, omit the line number; implies --trace"),
	UOPTION_DEF_D("trace-no-removed",      0, UOPT_NO_ARG,       "does not print removed readings; implies --trace"),
	UOPTION_DEF_D("trace-encl",            0, UOPT_NO_ARG,       "traces which enclosure pass is currently happening; implies --trace"),

	UOPTION_DEF_D("dry-run",               0, UOPT_NO_ARG,       "make no actual changes to the input"),
	UOPTION_DEF_D("single-run",            0, UOPT_NO_ARG,       "runs each section only once; same as --max-runs 1"),
	UOPTION_DEF_D("max-runs",              0, UOPT_REQUIRES_ARG, "runs each section max N times; defaults to unlimited (0)"),
	UOPTION_DEF_D("statistics",          'S', UOPT_NO_ARG,       "gathers profiling statistics while applying grammar"),
	UOPTION_DEF_D("optimize-unsafe",     'Z', UOPT_NO_ARG,       "destructively optimize the profiled grammar to be faster"),
	UOPTION_DEF_D("optimize-safe",       'z', UOPT_NO_ARG,       "conservatively optimize the profiled grammar to be faster"),
	UOPTION_DEF_D("prefix",              'p', UOPT_REQUIRES_ARG, "sets the mapping prefix; defaults to @"),
	UOPTION_DEF_D("unicode-tags",          0, UOPT_NO_ARG,       "outputs Unicode code points for things like ->"),
	UOPTION_DEF_D("unique-tags",           0, UOPT_NO_ARG,       "outputs unique tags only once per reading"),

	UOPTION_DEF_D("num-windows",           0, UOPT_REQUIRES_ARG, "number of windows to keep in before/ahead buffers; defaults to 2"),
	UOPTION_DEF_D("always-span",           0, UOPT_NO_ARG,       "forces scanning tests to always span across window boundaries"),
	UOPTION_DEF_D("soft-limit",            0, UOPT_REQUIRES_ARG, "number of cohorts after which the SOFT-DELIMITERS kick in; defaults to 300"),
	UOPTION_DEF_D("hard-limit",            0, UOPT_REQUIRES_ARG, "number of cohorts after which the window is forcefully cut; defaults to 500"),
	UOPTION_DEF_D("dep-delimit",         'D', UOPT_OPTIONAL_ARG, "delimit windows based on dependency instead of DELIMITERS; defaults to 10"),
	UOPTION_DEF_D("dep-original",          0, UOPT_NO_ARG,       "outputs the original input dependency tag even if it is no longer valid"),
	UOPTION_DEF_D("dep-allow-loops",       0, UOPT_NO_ARG,       "allows the creation of circular dependencies"),
	UOPTION_DEF_D("dep-no-crossing",       0, UOPT_NO_ARG,       "prevents the creation of dependencies that would result in crossing branches"),

	UOPTION_DEF_D("no-magic-readings",     0, UOPT_NO_ARG,       "prevents running rules on magic readings"),
	UOPTION_DEF_D("no-pass-origin",      'o', UOPT_NO_ARG,       "prevents scanning tests from passing the point of origin"),
	UOPTION_DEF_D("split-mappings",        0, UOPT_NO_ARG,       "keep mapped readings separate in output"),
	UOPTION_DEF_D("show-end-tags",       'e', UOPT_NO_ARG,       "allows the <<< tags to appear in output"),
	UOPTION_DEF_D("show-unused-sets",      0, UOPT_NO_ARG,       "prints a list of unused sets and their line numbers; implies --grammar-only"),
	UOPTION_DEF_D("show-tags",             0, UOPT_NO_ARG,       "prints a list of unique used tags; implies --grammar-only"),
	UOPTION_DEF_D("show-tag-hashes",       0, UOPT_NO_ARG,       "prints a list of tags and their hashes as they are parsed during the run"),
	UOPTION_DEF_D("show-set-hashes",       0, UOPT_NO_ARG,       "prints a list of sets and their hashes; implies --grammar-only"),
	UOPTION_DEF_D("dump-ast",              0, UOPT_NO_ARG,       "prints the grammar parse tree; implies --grammar-only"),
};
}

#endif
