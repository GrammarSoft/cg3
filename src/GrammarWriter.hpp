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
#ifndef c6d28b7452ec699b_GRAMMARWRITER_H
#define c6d28b7452ec699b_GRAMMARWRITER_H

#include "stdafx.hpp"

namespace CG3 {
class Grammar;
class Tag;
class Set;
class Rule;
class ContextualTest;

class GrammarWriter {
public:
	bool statistics;

	GrammarWriter(Grammar& res, std::ostream& ux_err);
	~GrammarWriter();

	int writeGrammar(std::ostream& output);

private:
	std::ostream* ux_stderr;
	uint32FlatHashSet used_sets;
	const Grammar* grammar;

	void printTag(std::ostream& out, const Tag& tag);
	void printSet(std::ostream& output, const Set& curset);
	void printRule(std::ostream& to, const Rule& rule);
	void printContextualTest(std::ostream& to, const ContextualTest& test);
};
}

#endif
