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

#include "Grammar.h"
#include "ContextualTest.h"

using namespace CG3;
using namespace CG3::Strings;

ContextualTest::ContextualTest() {
	pos = 0;
	offset = 0;
	target = 0;
	barrier = 0;
	cbarrier = 0;
	linked = 0;
	hash = 0;
	name = 0;
	line = 0;
	num_fail = 0;
	num_match = 0;
	total_time = 0;
	tmpl = 0;
	prev = 0;
	next = 0;
	is_used = false;
}

ContextualTest::~ContextualTest() {
	foreach (std::list<ContextualTest*>, ors, iter, iter_end) {
		delete *iter;
		*iter = 0;
	}
	detach();
	ors.clear();
	tmpl = 0;
	delete linked;
}

void ContextualTest::detach() {
	if (prev) {
		prev->next = next;
	}
	if (next) {
		next->prev = prev;
	}
	prev = next = 0;
}

void ContextualTest::parsePosition(const UChar *input, UFILE *ux_stderr) {
	if (u_strstr(input, stringbits[S_ASTERIKTWO])) {
		pos |= POS_SCANALL;
	}
	else if (u_strstr(input, stringbits[S_ASTERIK])) {
		pos |= POS_SCANFIRST;
	}
	if (u_strchr(input, 'C')) {
		pos |= POS_CAREFUL;
	}
	if (u_strchr(input, 'c')) {
		pos |= POS_DEP_CHILD;
	}
	if (u_strchr(input, 'p')) {
		pos |= POS_DEP_PARENT;
	}
	if (u_strchr(input, 's')) {
		pos |= POS_DEP_SIBLING;
	}
	if (u_strchr(input, 'S')) {
		pos |= POS_DEP_SELF;
	}
	if (u_strchr(input, '<')) {
		pos |= POS_SPAN_LEFT;
	}
	if (u_strchr(input, '>')) {
		pos |= POS_SPAN_RIGHT;
	}
	if (u_strchr(input, 'W')) {
		pos |= POS_SPAN_BOTH;
	}
	if (u_strchr(input, '@')) {
		pos |= POS_ABSOLUTE;
	}
	if (u_strchr(input, 'O')) {
		pos |= POS_NO_PASS_ORIGIN;
	}
	if (u_strchr(input, 'o')) {
		pos |= POS_PASS_ORIGIN;
	}
	if (u_strchr(input, 'L')) {
		pos |= POS_LEFT_PAR;
	}
	if (u_strchr(input, 'R')) {
		pos |= POS_RIGHT_PAR;
	}
	if (u_strchr(input, 'X')) {
		pos |= POS_MARK_SET;
	}
	if (u_strchr(input, 'x')) {
		pos |= POS_MARK_JUMP;
	}
	if (u_strchr(input, 'D')) {
		pos |= POS_LOOK_DELETED;
	}
	if (u_strchr(input, 'd')) {
		pos |= POS_LOOK_DELAYED;
	}
	if (u_strchr(input, '?')) {
		pos |= POS_NONE;
	}
	UChar tmp[16];
	tmp[0] = 0;
	int32_t retval = u_sscanf(input, "%[^0-9]%d", &tmp, &offset);
	if (u_strchr(input, '-')) {
		offset = (-1) * abs(offset);
	}

	if ((pos & (POS_DEP_CHILD|POS_DEP_SIBLING|POS_DEP_PARENT)) && (pos & (POS_SCANFIRST|POS_SCANALL))) {
		pos &= ~POS_SCANFIRST;
		pos &= ~POS_SCANALL;
		pos |= POS_DEP_DEEP;
	}
	if ((pos & (POS_DEP_CHILD|POS_DEP_SIBLING|POS_DEP_PARENT)) && (pos & POS_CAREFUL)) {
		pos &= ~POS_CAREFUL;
		pos |= POS_DEP_ALL;
	}
	if ((pos & (POS_DEP_CHILD|POS_DEP_SIBLING|POS_DEP_PARENT)) && (pos & POS_NEGATIVE)) {
		pos &= ~POS_NEGATIVE;
		pos |= POS_DEP_NONE;
	}

	if ((!(pos & (POS_DEP_CHILD|POS_DEP_SIBLING|POS_DEP_PARENT))) && (retval == EOF || (offset == 0 && tmp[0] == 0 && retval < 1))) {
		u_fprintf(ux_stderr, "Error: '%S' is not a valid position!\n", input);
		CG3Quit(1);
	}
	if ((pos & (POS_LEFT_PAR|POS_RIGHT_PAR)) && (pos & (POS_SCANFIRST|POS_SCANALL))) {
		u_fprintf(ux_stderr, "Error: '%S' is not a valid position - cannot have both enclosure and scan!\n", input);
		CG3Quit(1);
	}
	if ((pos & POS_PASS_ORIGIN) && (pos & POS_NO_PASS_ORIGIN)) {
		u_fprintf(ux_stderr, "Error: '%S' is not a valid position - cannot have both O and o!\n", input);
		CG3Quit(1);
	}
	if ((pos & POS_LEFT_PAR) && (pos & POS_RIGHT_PAR)) {
		u_fprintf(ux_stderr, "Error: '%S' is not a valid position - cannot have both L and R!\n", input);
		CG3Quit(1);
	}
	if ((pos & POS_DEP_ALL) && (pos & POS_DEP_NONE)) {
		u_fprintf(ux_stderr, "Error: '%S' is not a valid position - cannot have both NOT and C for dependencies!\n", input);
		CG3Quit(1);
	}
	if ((pos & POS_NONE) && (pos != POS_NONE || offset != 0 || u_strcmp(input, stringbits[S_QUESTION]) != 0)) {
		u_fprintf(ux_stderr, "Error: '%S' is not a valid position - '?' cannot be combined with anything else!\n", input);
		CG3Quit(1);
	}
}

ContextualTest *ContextualTest::allocateContextualTest() {
	return new ContextualTest;
}

uint32_t ContextualTest::rehash() {
	hash = 0;
	hash = hash_sdbm_uint32_t(hash, pos);
	hash = hash_sdbm_uint32_t(hash, target);
	hash = hash_sdbm_uint32_t(hash, barrier);
	hash = hash_sdbm_uint32_t(hash, abs(offset));
	if (offset < 0) {
		hash = hash_sdbm_uint32_t(hash, 5000);
	}
	if (linked) {
		hash = hash_sdbm_uint32_t(hash, linked->rehash());
	}
	return hash;
}

void ContextualTest::resetStatistics() {
	num_fail = 0;
	num_match = 0;
	total_time = 0;
	if (tmpl) {
		tmpl->resetStatistics();
	}
	foreach(std::list<ContextualTest*>, ors, idts, idts_end) {
		(*idts)->resetStatistics();
	}
	if (linked) {
		linked->resetStatistics();
	}
}

void ContextualTest::markUsed(Grammar *grammar) {
	is_used = true;

	Set *s = 0;
	if (target) {
		s = grammar->getSet(target);
		s->markUsed(grammar);
	}
	if (barrier) {
		s = grammar->getSet(barrier);
		s->markUsed(grammar);
	}
	if (cbarrier) {
		s = grammar->getSet(cbarrier);
		s->markUsed(grammar);
	}
	if (tmpl) {
		tmpl->markUsed(grammar);
	}
	foreach(std::list<ContextualTest*>, ors, idts, idts_end) {
		(*idts)->markUsed(grammar);
	}
	if (linked) {
		linked->markUsed(grammar);
	}
}

bool ContextualTest::cmp_quality(ContextualTest *a, ContextualTest *b) {
	return a->total_time > b->total_time;
}
