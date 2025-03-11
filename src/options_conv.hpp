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
#ifndef c6d28b7452ec699b_OPTIONS_CONV_H
#define c6d28b7452ec699b_OPTIONS_CONV_H

#include <uoptions.hpp>

namespace Options {
enum OPTIONS {
	HELP1,
	HELP2,
	MAPPING_PREFIX,
	IN_AUTO,
	IN_CG,
	IN_CG2,
	IN_NICELINE,
	IN_APERTIUM,
	IN_FST,
	IN_PLAIN,
	ADD_TAGS,
	OUT_CG,
	OUT_CG2,
	OUT_APERTIUM,
	OUT_FST,
	OUT_MATXIN,
	OUT_NICELINE,
	OUT_PLAIN,
	FST_WFACTOR,
	FST_WTAG,
	SUB_DELIMITER,
	SUB_RTL,
	SUB_LTR,
	ORDERED,
	PARSE_DEP,
	UNICODE_TAGS,
	PIPE_DELETED,
	NO_BREAK,
	NUM_OPTIONS,
};

std::array<UOption, NUM_OPTIONS> options{
	UOption{"help",         'h', UOPT_NO_ARG,       "shows this help"},
	UOption{"?",            '?', UOPT_NO_ARG,       "shows this help"},
	UOption{"prefix",       'p', UOPT_REQUIRES_ARG, "sets the mapping prefix; defaults to @"},
	UOption{"in-auto",      'u', UOPT_NO_ARG,       "auto-detect input format (default)"},
	UOption{"in-cg",        'c', UOPT_NO_ARG,       "sets input format to CG"},
	UOption{"v",            'v', UOPT_NO_ARG},
	UOption{"in-niceline",  'n', UOPT_NO_ARG,       "sets input format to Niceline CG"},
	UOption{"in-apertium",  'a', UOPT_NO_ARG,       "sets input format to Apertium"},
	UOption{"in-fst",       'f', UOPT_NO_ARG,       "sets input format to HFST/XFST"},
	UOption{"in-plain",     'x', UOPT_NO_ARG,       "sets input format to plain text"},
	UOption{"add-tags",       0, UOPT_NO_ARG,       "adds minimal analysis to readings (implies -x)"},
	UOption{"out-cg",       'C', UOPT_NO_ARG,       "sets output format to CG (default)"},
	UOption{"V",            'V', UOPT_NO_ARG},
	UOption{"out-apertium", 'A', UOPT_NO_ARG,       "sets output format to Apertium"},
	UOption{"out-fst",      'F', UOPT_NO_ARG,       "sets output format to HFST/XFST"},
	UOption{"out-matxin",   'M', UOPT_NO_ARG,       "sets output format to Matxin"},
	UOption{"out-niceline", 'N', UOPT_NO_ARG,       "sets output format to Niceline CG"},
	UOption{"out-plain",    'X', UOPT_NO_ARG,       "sets output format to plain text"},
	UOption{"wfactor",      'W', UOPT_REQUIRES_ARG, "FST weight factor (defaults to 1.0)"},
	UOption{"wtag",           0, UOPT_REQUIRES_ARG, "FST weight tag prefix (defaults to W)"},
	UOption{"sub-delim",    'S', UOPT_REQUIRES_ARG, "FST sub-reading delimiters (defaults to #)"},
	UOption{"rtl",          'r', UOPT_NO_ARG,       "sets sub-reading direction to RTL (default)"},
	UOption{"ltr",          'l', UOPT_NO_ARG,       "sets sub-reading direction to LTR"},
	UOption{"ordered",      'o', UOPT_NO_ARG,       "tag order matters mode"},
	UOption{"parse-dep",    'D', UOPT_NO_ARG,       "parse dependency (defaults to treating as normal tags)"},
	UOption{"unicode-tags",   0, UOPT_NO_ARG,       "outputs Unicode code points for things like ->"},
	UOption{"deleted",        0, UOPT_NO_ARG,       "read deleted readings as such, instead of as text"},
	UOption{"no-break",     'B', UOPT_NO_ARG,       "inhibits any extra whitespace in output"},
};

#include "options_parser.hpp"

}

#endif
