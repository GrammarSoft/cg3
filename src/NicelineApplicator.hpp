/*
* Copyright (C) 2007-2025, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_NICELINEAPPLICATOR_HPP
#define c6d28b7452ec699b_NICELINEAPPLICATOR_HPP

#include "GrammarApplicator.hpp"

namespace CG3 {

class NicelineApplicator : public virtual GrammarApplicator {
private:
	bool did_warn_statictags = false;
	bool did_warn_subreadings = false;

public:
	NicelineApplicator(std::ostream& ux_err);
	void runGrammarOnText(std::istream& input, std::ostream& output);

	void printReading(const Reading* reading, std::ostream& output);
	void printCohort(Cohort* cohort, std::ostream& output, bool profiling = false);
	void printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling = false);
};
}

#endif
