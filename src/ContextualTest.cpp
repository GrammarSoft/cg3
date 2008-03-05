/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */
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
	line = 0;
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}

ContextualTest::~ContextualTest() {
	delete linked;
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
	UChar tmp[16];
	tmp[0] = 0;
	int32_t retval = u_sscanf(input, "%[^0-9]%d", &tmp, &offset);
	if (u_strchr(input, '-')) {
		offset = (-1) * abs(offset);
	}

	if ((!(pos & (POS_DEP_CHILD|POS_DEP_SIBLING|POS_DEP_PARENT))) && (retval == EOF || (offset == 0 && tmp[0] == 0 && retval < 1))) {
		u_fprintf(ux_stderr, "Error: '%S' is not a valid position!\n", pos);
		CG3Quit(1);
	}
	if ((pos & (POS_DEP_CHILD|POS_DEP_SIBLING|POS_DEP_PARENT)) && (pos & (POS_SCANFIRST|POS_SCANALL))) {
		u_fprintf(ux_stderr, "Warning: Position '%S' is mixed. Behavior for mixed positions is undefined.\n", pos);
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
	assert(hash != 0);
	return hash;
}

void ContextualTest::resetStatistics() {
	num_fail = 0;
	num_match = 0;
	total_time = 0;
	if (linked) {
		linked->resetStatistics();
	}
}

bool ContextualTest::cmp_quality(ContextualTest *a, ContextualTest *b) {
	return a->total_time > b->total_time;
}
