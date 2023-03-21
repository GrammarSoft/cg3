/*
* Copyright (C) 2007-2023, GrammarSoft ApS
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

#include "Grammar.hpp"
#include "Rule.hpp"
#include "Strings.hpp"
#include "Tag.hpp"

namespace CG3 {

void Rule::setName(const UChar* to) {
	name.clear();
	if (to) {
		name = to;
	}
}

void Rule::addContextualTest(ContextualTest* to, ContextList& head) {
	head.push_front(to);
}

void Rule::reverseContextualTests() {
	tests.reverse();
	dep_tests.reverse();
}

}
