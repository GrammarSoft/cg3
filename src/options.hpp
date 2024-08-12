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

#pragma once
#ifndef c6d28b7452ec699b_OPTIONS_H
#define c6d28b7452ec699b_OPTIONS_H

#include <uoptions.hpp>

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
	NRULES,
	NRULES_INV,
	DODEBUG,
	DEBUG_RULES,
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
	PIPE_DELETED,
	DRYRUN,
	SINGLERUN,
	MAXRUNS,
	PROFILING,
	MAPPING_PREFIX,
	UNICODE_TAGS,
	UNIQUE_TAGS,
	NUM_WINDOWS,
	ALWAYS_SPAN,
	SOFT_LIMIT,
	HARD_LIMIT,
	TEXT_DELIMIT,
	DEP_DELIMIT,
	DEP_ABSOLUTE,
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
	NO_BREAK,
	NUM_OPTIONS,
};

std::array<UOption,NUM_OPTIONS> options{
	UOption{"help",                'h', UOPT_NO_ARG,       "shows this help"},
	UOption{"?",                   '?', UOPT_NO_ARG,       "shows this help"},
	UOption{"version",             'V', UOPT_NO_ARG,       "prints copyright and version information"},
	UOption{"min-binary-revision",   0, UOPT_NO_ARG,       "prints the minimum usable binary grammar revision"},
	UOption{"grammar",             'g', UOPT_REQUIRES_ARG, "specifies the grammar file to use for disambiguation"},
	UOption{"grammar-out",           0, UOPT_REQUIRES_ARG, "writes the compiled grammar in textual form to a file"},
	UOption{"grammar-bin",           0, UOPT_REQUIRES_ARG, "writes the compiled grammar in binary form to a file"},
	UOption{"grammar-only",          0, UOPT_NO_ARG,       "only compiles the grammar; implies --verbose"},
	UOption{"ordered",               0, UOPT_NO_ARG,       "(will in future allow full ordered matching)"},
	UOption{"unsafe",              'u', UOPT_NO_ARG,       "allows the removal of all readings in a cohort, even the last one"},
	UOption{"sections",            's', UOPT_REQUIRES_ARG, "number or ranges of sections to run; defaults to all sections"},
	UOption{"rules",                 0, UOPT_REQUIRES_ARG, "number or ranges of rules to run; defaults to all rules"},
	UOption{"rule",                  0, UOPT_REQUIRES_ARG, "a name or number of a single rule to run"},
	UOption{"nrules",                0, UOPT_REQUIRES_ARG, "a regex for which rule names to parse/run; defaults to all rules"},
	UOption{"nrules-v",              0, UOPT_REQUIRES_ARG, "a regex for which rule names not to parse/run"},
	UOption{"debug",               'd', UOPT_OPTIONAL_ARG, "enables debug output (very noisy)"},
	UOption{"debug-rules",           0, UOPT_REQUIRES_ARG, "number or ranges of rules to debug"},
	UOption{"verbose",             'v', UOPT_OPTIONAL_ARG, "increases verbosity"},
	UOption{"quiet",                 0, UOPT_NO_ARG,       "squelches warnings (same as -v 0)"},
	UOption{"vislcg-compat",       '2', UOPT_NO_ARG,       "enables compatibility mode for older CG-2 and vislcg grammars"},

	UOption{"stdin",               'I', UOPT_REQUIRES_ARG, "file to read input from instead of stdin"},
	UOption{"stdout",              'O', UOPT_REQUIRES_ARG, "file to print output to instead of stdout"},
	UOption{"stderr",              'E', UOPT_REQUIRES_ARG, "file to print errors to instead of stderr"},

	UOption{"codepage-all",        'C', UOPT_REQUIRES_ARG},
	UOption{"codepage-grammar",      0, UOPT_REQUIRES_ARG},
	UOption{"codepage-input",        0, UOPT_REQUIRES_ARG},
	UOption{"codepage-output",       0, UOPT_REQUIRES_ARG},

	UOption{"no-mappings",           0, UOPT_NO_ARG,       "disables all MAP, ADD, and REPLACE rules"},
	UOption{"no-corrections",        0, UOPT_NO_ARG,       "disables all SUBSTITUTE and APPEND rules"},
	UOption{"no-before-sections",    0, UOPT_NO_ARG,       "disables all rules in BEFORE-SECTIONS parts"},
	UOption{"no-sections",           0, UOPT_NO_ARG,       "disables all rules in SECTION parts"},
	UOption{"no-after-sections",     0, UOPT_NO_ARG,       "disables all rules in AFTER-SECTIONS parts"},

	UOption{"trace",               't', UOPT_OPTIONAL_ARG, "prints debug output alongside normal output; optionally stops execution"},
	UOption{"trace-name-only",       0, UOPT_NO_ARG,       "if a rule is named, omit the line number; implies --trace"},
	UOption{"trace-no-removed",      0, UOPT_NO_ARG,       "does not print removed readings; implies --trace"},
	UOption{"trace-encl",            0, UOPT_NO_ARG,       "traces which enclosure pass is currently happening; implies --trace"},

	UOption{"deleted",               0, UOPT_NO_ARG,       "read deleted readings as such, instead of as text"},

	UOption{"dry-run",               0, UOPT_NO_ARG,       "make no actual changes to the input"},
	UOption{"single-run",            0, UOPT_NO_ARG,       "runs each section only once; same as --max-runs 1"},
	UOption{"max-runs",              0, UOPT_REQUIRES_ARG, "runs each section max N times; defaults to unlimited (0)"},
	UOption{"profile",               0, UOPT_REQUIRES_ARG, "gathers profiling statistics and code coverage into a SQLite database"},
	UOption{"prefix",              'p', UOPT_REQUIRES_ARG, "sets the mapping prefix; defaults to @"},
	UOption{"unicode-tags",          0, UOPT_NO_ARG,       "outputs Unicode code points for things like ->"},
	UOption{"unique-tags",           0, UOPT_NO_ARG,       "outputs unique tags only once per reading"},

	UOption{"num-windows",           0, UOPT_REQUIRES_ARG, "number of windows to keep in before/ahead buffers; defaults to 2"},
	UOption{"always-span",           0, UOPT_NO_ARG,       "forces scanning tests to always span across window boundaries"},
	UOption{"soft-limit",            0, UOPT_REQUIRES_ARG, "number of cohorts after which the SOFT-DELIMITERS kick in; defaults to 300"},
	UOption{"hard-limit",            0, UOPT_REQUIRES_ARG, "number of cohorts after which the window is forcefully cut; defaults to 500"},
	UOption{"text-delimit",        'T', UOPT_OPTIONAL_ARG, "additional delimit based on non-CG text, ensuring it isn't attached to a cohort; defaults to /(^|\\n)</s/r"},
	UOption{"dep-delimit",         'D', UOPT_OPTIONAL_ARG, "delimit windows based on dependency instead of DELIMITERS; defaults to 10"},
	UOption{"dep-absolute",          0, UOPT_NO_ARG,       "outputs absolute cohort numbers rather than relative ones"},
	UOption{"dep-original",          0, UOPT_NO_ARG,       "outputs the original input dependency tag even if it is no longer valid"},
	UOption{"dep-allow-loops",       0, UOPT_NO_ARG,       "allows the creation of circular dependencies"},
	UOption{"dep-no-crossing",       0, UOPT_NO_ARG,       "prevents the creation of dependencies that would result in crossing branches"},

	UOption{"no-magic-readings",     0, UOPT_NO_ARG,       "prevents running rules on magic readings"},
	UOption{"no-pass-origin",      'o', UOPT_NO_ARG,       "prevents scanning tests from passing the point of origin"},
	UOption{"split-mappings",        0, UOPT_NO_ARG,       "keep mapped readings separate in output"},
	UOption{"show-end-tags",       'e', UOPT_NO_ARG,       "allows the <<< tags to appear in output"},
	UOption{"show-unused-sets",      0, UOPT_NO_ARG,       "prints a list of unused sets and their line numbers; implies --grammar-only"},
	UOption{"show-tags",             0, UOPT_NO_ARG,       "prints a list of unique used tags; implies --grammar-only"},
	UOption{"show-tag-hashes",       0, UOPT_NO_ARG,       "prints a list of tags and their hashes as they are parsed during the run"},
	UOption{"show-set-hashes",       0, UOPT_NO_ARG,       "prints a list of sets and their hashes; implies --grammar-only"},
	UOption{"dump-ast",              0, UOPT_NO_ARG,       "prints the grammar parse tree; implies --grammar-only"},
	UOption{"no-break",            'B', UOPT_NO_ARG,       "inhibits any extra whitespace in output"},
};

#include "options_parser.hpp"

}

#endif
