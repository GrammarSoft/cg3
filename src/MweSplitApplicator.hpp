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
#ifndef d01e916b65a71dd3_MWESPLITAPPLICATOR_HPP
#define d01e916b65a71dd3_MWESPLITAPPLICATOR_HPP

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"
#include "Cohort.hpp"

namespace CG3 {

class MweSplitApplicator : public virtual GrammarApplicator {
private:
	// bool did_warn_unhandled_situation;
	const Tag* maybeWfTag(const Reading* r);
	std::vector<Cohort*> splitMwe(Cohort* cohort);

public:
	MweSplitApplicator(std::ostream& ux_err);
	void runGrammarOnText(std::istream& input, std::ostream& output);

	void printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling = false);
};
}

#endif
