/*
* Copyright (C) 2006, GrammarSoft Aps
* and the VISL project at the University of Southern Denmark.
* All Rights Reserved.
*
* The contents of this file are subject to the GrammarSoft Public
* License Version 1.0 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.grammarsoft.com/GSPL or
* http://visl.sdu.dk/GSPL.txt
* 
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
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
		TRACE,
		REORDER,
		SINGLERUN,
		MAPPING_PREFIX,
		RE2C,
		NUM_WINDOWS,
		ALWAYS_SPAN,
		SOFT_LIMIT,
		HARD_LIMIT,
		DEP_DELIMIT,
		DEP_REENUM,
		DEP_HUMANIZE,
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
			UOPTION_DEF("trace",				0, UOPT_NO_ARG),
			UOPTION_DEF("reorder",				0, UOPT_NO_ARG),
			UOPTION_DEF("single-run",			0, UOPT_NO_ARG),
			UOPTION_DEF("prefix",			    0, UOPT_REQUIRES_ARG),

			UOPTION_DEF("re2c",					0, UOPT_NO_ARG),
			UOPTION_DEF("num-windows",			0, UOPT_REQUIRES_ARG),
			UOPTION_DEF("always-span",			0, UOPT_NO_ARG),
			UOPTION_DEF("soft-limit",			0, UOPT_REQUIRES_ARG),
			UOPTION_DEF("hard-limit",			0, UOPT_REQUIRES_ARG),
			UOPTION_DEF("dep-delimit",			0, UOPT_NO_ARG),
			UOPTION_DEF("dep-reenum",			0, UOPT_NO_ARG),
			UOPTION_DEF("dep-humanize",			0, UOPT_NO_ARG)
	};
}

#endif
