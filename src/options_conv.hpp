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
#ifndef c6d28b7452ec699b_OPTIONS_CONV_H
#define c6d28b7452ec699b_OPTIONS_CONV_H

#include <uoptions.h>

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
	OUT_MATXIN,
	OUT_NICELINE,
	OUT_PLAIN,
	FST_WFACTOR,
	FST_WTAG,
	SUB_DELIMITER,
	SUB_RTL,
	SUB_LTR,
	NUM_OPTIONS,
};

UOption options[] = {
	UOPTION_DEF_D("help",         'h', UOPT_NO_ARG,       "shows this help"),
	UOPTION_DEF_D("?",            '?', UOPT_NO_ARG,       "shows this help"),
	UOPTION_DEF_D("prefix",       'p', UOPT_REQUIRES_ARG, "sets the mapping prefix; defaults to @"),
	UOPTION_DEF_D("in-auto",      'u', UOPT_NO_ARG,       "auto-detect input format (default)"),
	UOPTION_DEF_D("in-cg",        'c', UOPT_NO_ARG,       "sets input format to CG"),
	UOPTION_DEF_D("v",            'v', UOPT_NO_ARG,       nullptr),
	UOPTION_DEF_D("in-niceline",  'n', UOPT_NO_ARG,       "sets input format to Niceline CG"),
	UOPTION_DEF_D("in-apertium",  'a', UOPT_NO_ARG,       "sets input format to Apertium"),
	UOPTION_DEF_D("in-fst",       'f', UOPT_NO_ARG,       "sets input format to HFST/XFST"),
	UOPTION_DEF_D("in-plain",     'p', UOPT_NO_ARG,       "sets input format to plain text"),
	UOPTION_DEF_D("add-tags",       0, UOPT_NO_ARG,       "adds minimal analysis to readings (implies -p)"),
	UOPTION_DEF_D("out-cg",       'C', UOPT_NO_ARG,       "sets output format to CG (default)"),
	UOPTION_DEF_D("V",            'V', UOPT_NO_ARG,       nullptr),
	UOPTION_DEF_D("out-apertium", 'A', UOPT_NO_ARG,       "sets output format to Apertium"),
	UOPTION_DEF_D("out-matxin",   'M', UOPT_NO_ARG,       "sets output format to Matxin"),
	UOPTION_DEF_D("out-niceline", 'N', UOPT_NO_ARG,       "sets output format to Niceline CG"),
	UOPTION_DEF_D("out-plain",    'P', UOPT_NO_ARG,       "sets output format to plain text"),
	UOPTION_DEF_D("wfactor",      'W', UOPT_REQUIRES_ARG, "FST weight factor (defaults to 1.0)"),
	UOPTION_DEF_D("wtag",           0, UOPT_REQUIRES_ARG, "FST weight tag prefix (defaults to W)"),
	UOPTION_DEF_D("sub-delim",    'S', UOPT_REQUIRES_ARG, "FST sub-reading delimiters (defaults to #)"),
	UOPTION_DEF_D("rtl",          'r', UOPT_NO_ARG,       "sets sub-reading direction to RTL (default)"),
	UOPTION_DEF_D("ltr",          'l', UOPT_NO_ARG,       "sets sub-reading direction to LTR"),
};
}

#endif
