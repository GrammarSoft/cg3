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

#pragma once
#ifndef __FORMATCONVERTER_H
#define __FORMATCONVERTER_H

#include "stdafx.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "ApertiumApplicator.h"

namespace CG3 {
	enum CG_FORMATS {
		FMT_VISL,
		FMT_APERTIUM,
		FMT_MATXIN,
		NUM_FORMATS
	};

	class FormatConverter : public ApertiumApplicator, public virtual GrammarApplicator {
	public:
		FormatConverter(UFILE *ux_err);

		virtual int runGrammarOnText(UFILE *input, UFILE *output);
		bool setInputFormat(CG_FORMATS format);
		bool setOutputFormat(CG_FORMATS format);

	protected:
		CG_FORMATS informat, outformat;
		virtual void printSingleWindow(SingleWindow *window, UFILE *output);
	};
}

#endif
