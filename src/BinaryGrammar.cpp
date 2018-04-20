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

#include "BinaryGrammar.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"

namespace CG3 {

BinaryGrammar::BinaryGrammar(Grammar& res, std::ostream& ux_err)
  : IGrammarParser(res, ux_err)
{
	grammar = result;
	verbosity = 0;
}

void BinaryGrammar::setCompatible(bool) {
}

void BinaryGrammar::setVerbosity(uint32_t v) {
	verbosity = v;
}

int BinaryGrammar::parse_grammar(const char* filename) {
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: Cannot parse into nothing - hint: call setResult() before trying.\n");
		CG3Quit(1);
	}

	struct stat _stat;
	int error = stat(filename, &_stat);

	if (error != 0) {
		u_fprintf(ux_stderr, "Error: Cannot stat %s due to error %d - bailing out!\n", filename, error);
		CG3Quit(1);
	}
	else {
		grammar->grammar_size = static_cast<size_t>(_stat.st_size);
	}

	std::ifstream input;
	input.exceptions(std::ios::failbit | std::ios::eofbit | std::ios::badbit);
	input.open(filename, std::ios::binary);

	return parse_grammar(input);
}
}
