/*
* Copyright (C) 2007-2017, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_BINARYGRAMMAR_H
#define c6d28b7452ec699b_BINARYGRAMMAR_H

#include "IGrammarParser.hpp"

namespace CG3 {
class ContextualTest;

class BinaryGrammar : public IGrammarParser {
public:
	BinaryGrammar(Grammar& result, UFILE *ux_err);

	int writeBinaryGrammar(FILE *output);
	int readBinaryGrammar(FILE *input);

	void setCompatible(bool compat);
	void setVerbosity(uint32_t level);
	int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);

private:
	Grammar *grammar;
	void writeContextualTest(ContextualTest *t, FILE *output);
	ContextualTest *readContextualTest(FILE *input);

	typedef std::unordered_map<ContextualTest*, uint32_t> deferred_t;
	deferred_t deferred_tmpls;
	typedef std::unordered_map<ContextualTest*, std::vector<uint32_t> > deferred_ors_t;
	deferred_ors_t deferred_ors;

	uint32FlatHashSet seen_uint32;

	int readBinaryGrammar_10043(FILE *input);
	ContextualTest *readContextualTest_10043(FILE *input);
};
}

#endif
