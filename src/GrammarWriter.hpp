/*
* Copyright (C) 2007-2023, GrammarSoft ApS
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
	GrammarWriter(Grammar& res, std::ostream& ux_err);
	~GrammarWriter();

	int writeGrammar(std::ostream& output);

private:
	std::ostream* ux_stderr = nullptr;
	uint32FlatHashSet used_sets;
	uint32FlatHashSet seen_rules;
	const Grammar* grammar = nullptr;
	std::multimap<uint32_t, uint32_t> anchors;

	void printTag(std::ostream& out, const Tag& tag);
	void printSet(std::ostream& output, const Set& curset);
	void printRule(std::ostream& to, const Rule& rule);
	void printContextualTest(std::ostream& to, const ContextualTest& test);
};
}

#endif
