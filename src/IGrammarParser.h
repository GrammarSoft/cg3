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
#ifndef __IGRAMMARPARSER_H
#define __IGRAMMARPARSER_H

#include "stdafx.h"
#include "Strings.h"
#include <unicode/uregex.h>
#include "Grammar.h"
#include "uextras.h"
#include <sys/stat.h>
 
namespace CG3 {
	class IGrammarParser {
	public:
		virtual void setCompatible(bool compat) = 0;
		virtual void setResult(CG3::Grammar *result) = 0;
		virtual int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage) = 0;
	};
}

#endif
