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
enum RULE_FLAGS : uint64_t {
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
	RF_IGNORED      = (1 << 29),
	RF_LOOKIGNORED  = (1 << 30),
	RF_NOMAPPED     = (1ull << 31),
	RF_NOPARENT     = (1ull << 32),
	RF_DETACH       = (1ull << 33),
};

using rule_flags_t = std::underlying_type<RULE_FLAGS>::type;

constexpr rule_flags_t flag_excls[] = {
	RF_NEAREST | RF_ALLOWLOOP,
	RF_DELAYED | RF_IMMEDIATE | RF_IGNORED,
	RF_UNSAFE | RF_SAFE,
	RF_REMEMBERX | RF_RESETX,
	RF_KEEPORDER | RF_VARYORDER,
	RF_ENCL_INNER | RF_ENCL_OUTER | RF_ENCL_FINAL | RF_ENCL_ANY,
	RF_WITHCHILD | RF_NOCHILD,
	RF_ITERATE | RF_NOITERATE,
	RF_BEFORE | RF_AFTER,
};

constexpr auto init_flag_excls(rule_flags_t v) {
	for (auto excl : flag_excls) {
		if (excl & (static_cast<rule_flags_t>(1) << v)) {
			return excl;
		}
	}
	return static_cast<rule_flags_t>(0);
}

constexpr auto _flags_excls = make_array<FLAGS_COUNT>(init_flag_excls);

class Rule;
typedef std::vector<Rule*> RuleVector;

class Rule {
public:
	UString name;
	Tag* wordform = nullptr;
	uint32_t target = 0;
	uint32_t childset1 = 0, childset2 = 0;
	uint32_t line = 0, number = 0;
	uint32_t varname = 0, varvalue = 0; // ToDo: varvalue is unused
	uint64_t flags = 0;
	int32_t section = 0;
	int32_t sub_reading = 0;
	KEYWORDS type = K_IGNORE;
	Set* maplist = nullptr;
	Set* sublist = nullptr;
	RuleVector sub_rules;

	mutable ContextList tests;
	mutable ContextList dep_tests;
	mutable ContextualTest* dep_target = nullptr;

	Rule() = default;
	~Rule() = default;
	void setName(const UChar* to);

	void addContextualTest(ContextualTest* to, ContextList& head);
	void reverseContextualTests();
};

typedef std::map<uint32_t, Rule*> RuleByLineMap;
typedef std::unordered_map<uint32_t, Rule*> RuleByLineHashMap;
}

#endif
