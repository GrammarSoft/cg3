/*
* Copyright (C) 2007-2021, GrammarSoft ApS
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

void Rule::resetStatistics() {
	for (auto it : tests) {
		it->resetStatistics();
	}
	for (auto it : dep_tests) {
		it->resetStatistics();
	}
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}

bool Rule::cmp_quality(const Rule* a, const Rule* b) {
	return a->total_time > b->total_time;
}
}
