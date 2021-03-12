/*
* Copyright (C) 2007-2020, GrammarSoft ApS
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

enum ApertiumCasing { Lower, Title, Upper };

class ApertiumApplicator : public virtual GrammarApplicator {
public:
	ApertiumApplicator(std::ostream& ux_err);

	void runGrammarOnText(std::istream& input, std::ostream& output);

	bool wordform_case;
	bool print_word_forms;
	bool print_only_first;
	bool delimit_lexical_units; // Should cohorts be surrounded by ^$ ?
	bool surface_readings;	    // Should readings have escaped symbols as if they were surface forms?

	void testPR(std::ostream& output);

protected:
	void printReading(Reading* reading, std::ostream& output);
	void printSingleWindow(SingleWindow* window, std::ostream& output);

	void mergeMappings(Cohort& cohort);

private:
	/**
	 * Parse a stream variable from cleaned blank.
	 *
	 * Tries not to do anything more than what happens in GrammarApplicator_runGrammar.cpp, for
	 * easy merging.
	 *
	 * @param cleaned something like "<STREAMCMD:SETVAR:forskjell_skilnad" (note: no trailing >).
	 */
	void parseStreamVar(const SingleWindow* cSWindow, UString& cleaned,
			    uint32FlatHashMap& variables_set, uint32FlatHashSet& variables_rem, uint32SortedVector& variables_output);
	void printReading(Reading* reading, std::ostream& output, ApertiumCasing casing, int firstlower);
	void processReading(Reading* cReading, UChar* reading_string, Tag* wform);
	void processReading(Reading* cReading, UString& reading_string, Tag* wform);
};
}

#endif
