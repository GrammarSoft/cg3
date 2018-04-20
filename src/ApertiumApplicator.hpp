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
#ifndef c6d28b7452ec699b_GRAMMARAPPLICATORAPERTIUM_H
#define c6d28b7452ec699b_GRAMMARAPPLICATORAPERTIUM_H

#include "GrammarApplicator.hpp"

namespace CG3 {
class ApertiumApplicator : public virtual GrammarApplicator {
public:
	ApertiumApplicator(std::ostream& ux_err);

	void runGrammarOnText(std::istream& input, std::ostream& output);

	bool wordform_case;
	bool print_word_forms;
	bool print_only_first;
	void setNullFlush(bool pNullFlush);

	void testPR(std::ostream& output);

protected:
	bool nullFlush;
	bool runningWithNullFlush;

	void printReading(Reading* reading, std::ostream& output);
	void printSingleWindow(SingleWindow* window, std::ostream& output);

	void runGrammarOnTextWrapperNullFlush(std::istream& input, std::ostream& output);

	void mergeMappings(Cohort& cohort);

private:
	void processReading(Reading* cReading, const UChar* reading_string);
	void processReading(Reading* cReading, const UString& reading_string);
};
}

#endif
