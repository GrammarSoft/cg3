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
#ifndef c6d28b7452ec699b_RULE_H
#define c6d28b7452ec699b_RULE_H

#include "stdafx.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Cohort.hpp"
#include "ContextualTest.hpp"

namespace CG3 {

class Grammar;
class Set;

// This must be kept in lock-step with Strings.hpp's FLAGS
enum {
	RF_NEAREST      = (1 <<  0),
	RF_ALLOWLOOP    = (1 <<  1),
	RF_DELAYED      = (1 <<  2),
	RF_IMMEDIATE    = (1 <<  3),
	RF_LOOKDELETED  = (1 <<  4),
	RF_LOOKDELAYED  = (1 <<  5),
	RF_UNSAFE       = (1 <<  6),
	RF_SAFE         = (1 <<  7),
	RF_REMEMBERX    = (1 <<  8),
	RF_RESETX       = (1 <<  9),
	RF_KEEPORDER    = (1 << 10),
	RF_VARYORDER    = (1 << 11),
	RF_ENCL_INNER   = (1 << 12),
	RF_ENCL_OUTER   = (1 << 13),
	RF_ENCL_FINAL   = (1 << 14),
	RF_ENCL_ANY     = (1 << 15),
	RF_ALLOWCROSS   = (1 << 16),
	RF_WITHCHILD    = (1 << 17),
	RF_NOCHILD      = (1 << 18),
	RF_ITERATE      = (1 << 19),
	RF_NOITERATE    = (1 << 20),
	RF_UNMAPLAST    = (1 << 21),
	RF_REVERSE      = (1 << 22),
	RF_SUB          = (1 << 23),
	RF_OUTPUT       = (1 << 24),
	RF_CAPTURE_UNIF = (1 << 25),
	RF_REPEAT       = (1 << 26),
	RF_BEFORE       = (1 << 27),
	RF_AFTER        = (1 << 28),

	MASK_ENCL       = RF_ENCL_INNER | RF_ENCL_OUTER | RF_ENCL_FINAL | RF_ENCL_ANY,
};

class Rule {
public:
	UString name;
	Tag* wordform;
	uint32_t target;
	uint32_t childset1, childset2;
	uint32_t line, number;
	uint32_t varname, varvalue; // ToDo: varvalue is unused
	uint32_t flags;
	int32_t section;
	int32_t sub_reading;
	KEYWORDS type;
	Set* maplist;
	Set* sublist;

	mutable ContextList tests;
	mutable ContextList dep_tests;
	mutable uint32_t num_fail, num_match;
	mutable double total_time;
	mutable ContextualTest* dep_target;

	Rule();
	~Rule();
	void setName(const UChar* to);

	void resetStatistics();

	void addContextualTest(ContextualTest* to, ContextList& head);
	void reverseContextualTests();

	static bool cmp_quality(const Rule* a, const Rule* b);
};

typedef std::vector<Rule*> RuleVector;
typedef std::map<uint32_t, Rule*> RuleByLineMap;
typedef std::unordered_map<uint32_t, Rule*> RuleByLineHashMap;
}

#endif
