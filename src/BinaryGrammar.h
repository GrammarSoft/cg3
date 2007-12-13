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
#ifndef __BINARYGRAMMAR_H
#define __BINARYGRAMMAR_H

#include "IGrammarParser.h"
#include "version.h"
 
namespace CG3 {
	class BinaryGrammar : public IGrammarParser {
	public:
		BinaryGrammar(Grammar *res, UFILE *ux_err);
		~BinaryGrammar();

		int writeBinaryGrammar(FILE *output);
		int readBinaryGrammar(FILE *input);

		void setCompatible(bool compat);
		void setResult(CG3::Grammar *result);
		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);
	private:
		UFILE *ux_stderr;
		Grammar *grammar;
		void writeContextualTest(ContextualTest *t, FILE *output);
		void readContextualTest(ContextualTest *t, FILE *input);
	};
}

#endif
