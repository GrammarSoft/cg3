/*
* Copyright (C) 2007-2024, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_GRAMMARAPPLICATORMATXIN_H
#define c6d28b7452ec699b_GRAMMARAPPLICATORMATXIN_H

#include "GrammarApplicator.hpp"

namespace CG3 {
class MatxinApplicator : public virtual GrammarApplicator {
public:
	MatxinApplicator(std::ostream& ux_err);

	void runGrammarOnText(std::istream& input, std::ostream& output);

	bool getNullFlush();
	bool wordform_case = false;
	bool print_word_forms = true;
	bool print_only_first = false;
	void setNullFlush(bool pNullFlush);

	void testPR(std::ostream& output);

protected:
	struct Node {
		int self = 0;
		UString lemma;
		UString form;
		UString pos;
		UString mi;
		UString si;
	};

	std::map<int, Node> nodes;
	std::map<int, std::vector<int>> deps;

	bool nullFlush = false;
	bool runningWithNullFlush = false;

	void printReading(Reading* reading, Node& n, std::ostream& output);
	void printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling = false);

	void procNode(int& depth, std::map<int, Node>& nodes, std::map<int, std::vector<int>>& deps, int node, std::ostream& output);

	void runGrammarOnTextWrapperNullFlush(std::istream& input, std::ostream& output);

	void mergeMappings(Cohort& cohort);

private:
	void processReading(Reading* cReading, const UChar* reading_string);
	void processReading(Reading* cReading, const UString& reading_string);
};
}

#endif
