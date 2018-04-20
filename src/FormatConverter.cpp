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

#include "FormatConverter.hpp"

namespace CG3 {

FormatConverter::FormatConverter(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
  , ApertiumApplicator(ux_err)
  , NicelineApplicator(ux_err)
  , PlaintextApplicator(ux_err)
  , FSTApplicator(ux_err)
  , MatxinApplicator(ux_err)
  , informat(FMT_CG)
  , outformat(FMT_CG)
{
}

void FormatConverter::setInputFormat(CG_FORMATS format) {
	informat = format;
}

void FormatConverter::setOutputFormat(CG_FORMATS format) {
	outformat = format;
}

void FormatConverter::runGrammarOnText(std::istream& input, std::ostream& output) {
	switch (informat) {
	case FMT_CG: {
		GrammarApplicator::runGrammarOnText(input, output);
		break;
	}
	case FMT_APERTIUM: {
		ApertiumApplicator::runGrammarOnText(input, output);
		break;
	}
	case FMT_NICELINE: {
		NicelineApplicator::runGrammarOnText(input, output);
		break;
	}
	case FMT_PLAIN: {
		PlaintextApplicator::runGrammarOnText(input, output);
		break;
	}
	case FMT_FST: {
		FSTApplicator::runGrammarOnText(input, output);
		break;
	}
	default:
		CG3Quit();
	}
}

void FormatConverter::printSingleWindow(SingleWindow* window, std::ostream& output) {
	switch (outformat) {
	case FMT_CG: {
		GrammarApplicator::printSingleWindow(window, output);
		break;
	}
	case FMT_APERTIUM: {
		ApertiumApplicator::printSingleWindow(window, output);
		break;
	}
	case FMT_NICELINE: {
		NicelineApplicator::printSingleWindow(window, output);
		break;
	}
	case FMT_PLAIN: {
		PlaintextApplicator::printSingleWindow(window, output);
		break;
	}
	default:
		CG3Quit();
	}
}
}
