/*
* Copyright (C) 2007-2011, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_GRAMMARAPPLICATORAPERTIUM_H
#define c6d28b7452ec699b_GRAMMARAPPLICATORAPERTIUM_H

#include "stdafx.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "GrammarApplicator.h"

namespace CG3 {
	class ApertiumApplicator : public virtual GrammarApplicator {
	public:
		ApertiumApplicator(UFILE *ux_err);

		virtual int runGrammarOnText(UFILE *input, UFILE *output);

		bool getNullFlush();
		bool wordform_case;
		bool print_word_forms;
		void setNullFlush(bool pNullFlush);

		void testPR(UFILE *output);
		
	protected:
		bool nullFlush;
		bool runningWithNullFlush;
	
		void printReading(Reading *reading, UFILE *output);
		virtual void printSingleWindow(SingleWindow *window, UFILE *output);
		
		int runGrammarOnTextWrapperNullFlush(UFILE *input, UFILE *output);

		UChar u_fgetc_wrapper(UFILE *input);
		UConverter* fgetc_converter;
		char fgetc_inputbuf[5];
		UChar fgetc_outputbuf[5];
		UErrorCode fgetc_error;
		void mergeMappings(Cohort& cohort);
		
	private:

		void processReading(Reading *cReading, const UChar *reading_string);
		void processReading(Reading *cReading, const UString& reading_string);

	};
}

#endif
