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
#ifndef c6d28b7452ec699b_FORMATCONVERTER_H
#define c6d28b7452ec699b_FORMATCONVERTER_H

#include "ApertiumApplicator.hpp"
#include "MatxinApplicator.hpp"
#include "NicelineApplicator.hpp"
#include "PlaintextApplicator.hpp"
#include "FSTApplicator.hpp"

namespace CG3 {
enum CG_FORMATS {
	FMT_INVALID,
	FMT_CG,
	FMT_NICELINE,
	FMT_APERTIUM,
	FMT_MATXIN,
	FMT_FST,
	FMT_PLAIN,
	NUM_FORMATS,
};

class FormatConverter : public ApertiumApplicator, public NicelineApplicator, public PlaintextApplicator, public FSTApplicator, public MatxinApplicator {
public:
	FormatConverter(std::ostream& ux_err);

	void runGrammarOnText(std::istream& input, std::ostream& output);
	void setInputFormat(CG_FORMATS format);
	void setOutputFormat(CG_FORMATS format);

protected:
	CG_FORMATS informat, outformat;
	void printSingleWindow(SingleWindow* window, std::ostream& output);
};
}

#endif
