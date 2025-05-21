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
#ifndef c6d28b7452ec699b_FORMATCONVERTER_H
#define c6d28b7452ec699b_FORMATCONVERTER_H

#include "ApertiumApplicator.hpp"
#include "FSTApplicator.hpp"
#include "JsonlApplicator.hpp"
#include "MatxinApplicator.hpp"
#include "NicelineApplicator.hpp"
#include "PlaintextApplicator.hpp"
#include "Grammar.hpp"
#include "cg3.h"

namespace CG3 {

cg3_sformat detectFormat(std::string_view str);

class FormatConverter : public ApertiumApplicator, public FSTApplicator, public JsonlApplicator, public MatxinApplicator, public NicelineApplicator, public PlaintextApplicator {
public:
	FormatConverter(std::ostream& ux_err);

	void runGrammarOnText(std::istream& input, std::ostream& output);

	std::unique_ptr<std::istream> detectFormat(std::istream& in);
	cg3_sformat fmt_input = CG3SF_CG;
	cg3_sformat fmt_output = CG3SF_CG;

	Grammar conv_grammar;

protected:
	void printCohort(Cohort* cohort, std::ostream& output, bool profiling = false) override;
	void printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling = false) override;
	void printStreamCommand(UStringView cmd, std::ostream& output) override;
	void printPlainTextLine(UStringView line, std::ostream& output) override;
};

}

#endif
