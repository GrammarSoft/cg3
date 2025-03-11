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
#ifndef c6d28b7452ec699b_IGRAMMARPARSER_H
#define c6d28b7452ec699b_IGRAMMARPARSER_H

#include "stdafx.hpp"

namespace CG3 {
class Grammar;

class IGrammarParser {
public:
	IGrammarParser(Grammar& res, std::ostream& ux_err)
	  : ux_stderr(&ux_err)
	  , result(&res)
	{
	}

	virtual ~IGrammarParser() {
		if (nrules) {
			uregex_close(nrules);
		}
		if (nrules_inv) {
			uregex_close(nrules_inv);
		}
	}
	virtual void setCompatible(bool compat) = 0;
	virtual void setVerbosity(uint32_t level) = 0;
	virtual int parse_grammar(const char* buffer, size_t length) = 0;
	virtual int parse_grammar(const UChar* buffer, size_t length) = 0;
	virtual int parse_grammar(const std::string& buffer) = 0;
	virtual int parse_grammar(const char* filename) = 0;

	std::ostream* ux_stderr = nullptr;
	URegularExpression* nrules = nullptr;
	URegularExpression* nrules_inv = nullptr;

protected:
	virtual int parse_grammar(UString& buffer) = 0;

	Grammar* result = nullptr;
	uint32_t verbosity = 0;
};
}

#endif
