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
#ifndef c6d28b7452ec699b_PLAINTEXTAPPLICATOR_HPP
#define c6d28b7452ec699b_PLAINTEXTAPPLICATOR_HPP

#include "GrammarApplicator.hpp"

namespace CG3 {

class PlaintextApplicator : public virtual GrammarApplicator {
public:
	bool add_tags = false;

	PlaintextApplicator(std::ostream& ux_err);

	void runGrammarOnText(std::istream& input, std::ostream& output) override;

protected:
	void printCohort(Cohort* cohort, std::ostream& output, bool profiling = false) override;
	void printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling = false) override;
};
}

#endif
