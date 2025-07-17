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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_OPTIONS_H
#define c6d28b7452ec699b_OPTIONS_H

#include "stdafx.hpp"
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
	PRINT_IDS,
	PRINT_DEP,
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

using options_t = std::array<UOption,NUM_OPTIONS>;

CG3_IMPORTS extern options_t options;

CG3_IMPORTS extern options_t options_default;
CG3_IMPORTS extern options_t options_override;

CG3_IMPORTS extern options_t grammar_options_default;
CG3_IMPORTS extern options_t grammar_options_override;

}

#endif
