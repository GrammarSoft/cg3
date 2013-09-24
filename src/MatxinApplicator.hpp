/*
* Copyright (C) 2007-2013, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_GRAMMARAPPLICATORMATXIN_H
#define c6d28b7452ec699b_GRAMMARAPPLICATORMATXIN_H

#include "stdafx.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "GrammarApplicator.hpp"

namespace CG3 {
	class MatxinApplicator : public virtual GrammarApplicator {
	public:
		MatxinApplicator(UFILE *ux_err);

		void runGrammarOnText(istream& input, UFILE *output);

		bool getNullFlush();
		bool wordform_case;
		bool print_word_forms;
		void setNullFlush(bool pNullFlush);
		// Readings with this tag get their own chunk:
		const UChar *CHUNK;
		
	protected:
		bool nullFlush;
		bool runningWithNullFlush;
	
		int printReading(Reading *reading, UFILE *output, int ischunk, int ord, int alloc);
		void printSingleWindow(SingleWindow *window, UFILE *output);
		
		void runGrammarOnTextWrapperNullFlush(istream& input, UFILE *output);

		UChar u_fgetc_wrapper(istream& input);
		UConverter* fgetc_converter;
		char fgetc_inputbuf[5];
		UChar fgetc_outputbuf[5];
		UErrorCode fgetc_error;

	private:
		size_t window_alloc;
		void processReading(Reading *cReading, const UChar *reading_string);
		void processReading(Reading *cReading, const UString& reading_string);

	};
}

#endif
