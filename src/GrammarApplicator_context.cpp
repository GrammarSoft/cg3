/*
* Copyright (C) 2007-2023, GrammarSoft ApS
* Developed by Daniel Swanson <awesomeevildudes@gmail.com>, Tino Didriksen <mail@tinodidriksen.com>
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

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"
#include "ContextualTest.hpp"

namespace CG3 {

ReadingSpec GrammarApplicator::get_attach_to() {
	if (context_stack.empty()) {
		ReadingSpec ret;
		return ret;
	}
	else {
		return context_stack.back().attach_to;
	}
}

Cohort* GrammarApplicator::get_mark() {
	if (context_stack.empty()) return nullptr;
	else return context_stack.back().mark;
}

ReadingSpec GrammarApplicator::get_apply_to() {
	if (context_stack.empty()) {
		ReadingSpec ret;
		return ret;
	}
	else if (context_stack.back().attach_to.cohort != nullptr) {
		return context_stack.back().attach_to;
	}
	else {
		return context_stack.back().target;
	}
}

void GrammarApplicator::set_attach_to(Reading* reading, Reading* subreading) {
	if (!context_stack.empty()) {
		auto& spec = context_stack.back().attach_to;
		spec.cohort = reading->parent;
		spec.reading = reading;
		spec.subreading = subreading;
	}
}

void GrammarApplicator::set_mark(Cohort* cohort) {
	if (!context_stack.empty()) {
		context_stack.back().mark = cohort;
	}
}

bool GrammarApplicator::check_unif_tags(uint32_t set, const void* val) {
	if (context_stack.empty()) return false;
	auto& unif_tags = *(context_stack.back().unif_tags);
	auto it = unif_tags.find(set);
	if (it != unif_tags.end()) {
		return it->second == val;
	}
	unif_tags[set] = val;
	return true;
}
}
