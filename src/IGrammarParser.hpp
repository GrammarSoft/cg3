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

	virtual ~IGrammarParser() {}
	virtual void setCompatible(bool compat) = 0;
	virtual void setVerbosity(uint32_t level) = 0;
	virtual int parse_grammar(const char* buffer, size_t length) = 0;
	virtual int parse_grammar(const UChar* buffer, size_t length) = 0;
	virtual int parse_grammar(const std::string& buffer) = 0;
	virtual int parse_grammar(const char* filename) = 0;

	std::ostream* ux_stderr = 0;

protected:
	virtual int parse_grammar(UString& buffer) = 0;

	Grammar* result = 0;
	uint32_t verbosity = 0;
};
}

#endif
