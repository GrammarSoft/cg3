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
#ifndef c6d28b7452ec699b_BINARYGRAMMAR_H
#define c6d28b7452ec699b_BINARYGRAMMAR_H

#include "IGrammarParser.hpp"

namespace CG3 {
class ContextualTest;

class BinaryGrammar : public IGrammarParser {
public:
	BinaryGrammar(Grammar& result, std::ostream& ux_err);

	int writeBinaryGrammar(FILE* output);

	void setCompatible(bool compat);
	void setVerbosity(uint32_t level);
	int parse_grammar(std::istream& input);
	int parse_grammar(const char* buffer, size_t length);
	int parse_grammar(const UChar* buffer, size_t length);
	int parse_grammar(const std::string& buffer);
	int parse_grammar(const char* filename);

private:
	int parse_grammar(UString& buffer);

	Grammar* grammar;
	void writeContextualTest(ContextualTest* t, FILE* output);
	ContextualTest* readContextualTest(std::istream& input);

	typedef std::unordered_map<ContextualTest*, uint32_t> deferred_t;
	deferred_t deferred_tmpls;
	typedef std::unordered_map<ContextualTest*, std::vector<uint32_t>> deferred_ors_t;
	deferred_ors_t deferred_ors;

	uint32FlatHashSet seen_uint32;

	int readBinaryGrammar_10043(std::istream& input);
	ContextualTest* readContextualTest_10043(std::istream& input);
};
}

#endif
