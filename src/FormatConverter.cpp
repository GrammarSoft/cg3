/*
* Copyright (C) 2007, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
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

#include "FormatConverter.h"

namespace CG3 {

FormatConverter::FormatConverter(UFILE *ux_err) :
GrammarApplicator(ux_err),
ApertiumApplicator(ux_err),
informat(FMT_VISL),
outformat(FMT_VISL)
{
}

bool FormatConverter::setInputFormat(CG_FORMATS format) {
	informat = format;
	return true;
}

bool FormatConverter::setOutputFormat(CG_FORMATS format) {
	outformat = format;
	return true;
}

int FormatConverter::runGrammarOnText(UFILE *input, UFILE *output) {
	switch (informat) {
		case FMT_VISL: {
			GrammarApplicator::runGrammarOnText(input, output);
			break;
		}
		case FMT_APERTIUM: {
			ApertiumApplicator::runGrammarOnText(input, output);
			break;
		}
		case FMT_MATXIN:
		default:
			CG3Quit();
	}

	return 0;
}

void FormatConverter::printSingleWindow(SingleWindow *window, UFILE *output) {
	switch (outformat) {
		case FMT_VISL: {
			GrammarApplicator::printSingleWindow(window, output);
			break;
		}
		case FMT_APERTIUM: {
			ApertiumApplicator::printSingleWindow(window, output);
			break;
		}
		case FMT_MATXIN:
		default:
			CG3Quit();
	}
}

}
