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
#ifndef __GRAMMARWRITER_H
#define __GRAMMARWRITER_H

#include "stdafx.h"
#include <unicode/uchar.h>
#include <unicode/ustdio.h>
#include "Grammar.h"
 
namespace CG3 {
	class GrammarWriter {
	public:
		CG3::Grammar *grammar;
	
		GrammarWriter();
		~GrammarWriter();

		int write_grammar_to_ufile_text(UFILE *output);
		int write_grammar_to_ufile_binary(UFILE *output);

		void setGrammar(CG3::Grammar *res);
	};
}

#endif
