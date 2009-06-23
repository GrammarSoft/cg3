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

#pragma once
#ifndef __BINARYGRAMMAR_H
#define __BINARYGRAMMAR_H

#include "IGrammarParser.h"

namespace CG3 {
	class BinaryGrammar : public IGrammarParser {
	public:
		BinaryGrammar(Grammar &result, UFILE *ux_err);
		~BinaryGrammar();

		int writeBinaryGrammar(FILE *output);
		int readBinaryGrammar(FILE *input);

		void setCompatible(bool compat);
		void setVerbosity(uint32_t level);
		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);
	private:
		Grammar *grammar;
		void writeContextualTest(ContextualTest *t, FILE *output);
		void readContextualTest(ContextualTest *t, FILE *input);
	};
}

#endif
