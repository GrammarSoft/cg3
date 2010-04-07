/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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

#pragma once
#ifndef __STRINGS_H
#define __STRINGS_H

#include "stdafx.h"

namespace CG3 {
	// ToDo: Add ABORT
	enum KEYWORDS {
		K_IGNORE,
		K_SETS,
		K_LIST,
		K_SET,
		K_DELIMITERS,
		K_SOFT_DELIMITERS,
		K_PREFERRED_TARGETS,
		K_MAPPING_PREFIX,
		K_MAPPINGS,
		K_CONSTRAINTS,
		K_CORRECTIONS,
		K_SECTION,
		K_BEFORE_SECTIONS,
		K_AFTER_SECTIONS,
		K_NULL_SECTION,
		K_ADD,
		K_MAP,
		K_REPLACE,
		K_SELECT,
		K_REMOVE,
		K_IFF,
		K_APPEND,
		K_SUBSTITUTE,
		K_START,
		K_END,
		K_ANCHOR,
		K_EXECUTE,
		K_JUMP,
		K_REMVARIABLE,
		K_SETVARIABLE,
		K_DELIMIT,
		K_MATCH,
		K_SETPARENT,
		K_SETCHILD,
		K_ADDRELATION,
		K_SETRELATION,
		K_REMRELATION,
		K_ADDRELATIONS,
		K_SETRELATIONS,
		K_REMRELATIONS,
		K_TEMPLATE,
		K_MOVE,
		K_MOVE_AFTER,
		K_MOVE_BEFORE,
		K_SWITCH,
		KEYWORD_COUNT
	};

	enum STRINGS {
		S_IGNORE,
		S_PIPE,
		S_TO,
		S_OR,
		S_PLUS,
		S_MINUS,
		S_MULTIPLY,
		S_ASTERIK = S_MULTIPLY,
		S_ASTERIKTWO,
		S_FAILFAST,
		S_BACKSLASH,
		S_HASH,
		S_NOT,
		S_TEXTNOT,
		S_TEXTNEGATE,
		S_ALL,
		S_NONE,
		S_LINK,
		S_BARRIER,
		S_CBARRIER,
		S_CMD_FLUSH,
		S_CMD_EXIT,
		S_CMD_IGNORE,
		S_CMD_RESUME,
		S_TARGET,
		S_AND,
		S_IF,
		S_DELIMITSET,
		S_SOFTDELIMITSET,
		S_BEGINTAG,
		S_ENDTAG,
		S_LINKZ,
		S_SPACE,
		S_UU_LEFT,
		S_UU_RIGHT,
		S_UU_PAREN,
		S_UU_TARGET,
		S_UU_MARK,
		S_UU_ATTACHTO,
		S_RXTEXT_ANY,
		S_RXBASE_ANY,
		S_RXWORD_ANY,
		S_AFTER,
		S_BEFORE,
		S_WITH,
		S_QUESTION,
		S_VS1,
		S_VS2,
		S_VS3,
		S_VS4,
		S_VS5,
		S_VS6,
		S_VS7,
		S_VS8,
		S_VS9,
		S_VSu,
		S_VSU,
		S_VSl,
		S_VSL,
		S_GPREFIX,
		S_POSITIVE,
		S_NEGATIVE,
		STRINGS_COUNT
	};

	// This must be kept in lock-step with Rule.h's RULE_FLAGS
	enum FLAGS {
		FL_NEAREST,
		FL_ALLOWLOOP,
		FL_DELAYED,
		FL_IMMEDIATE,
		FL_LOOKDELETED,
		FL_LOOKDELAYED,
		FL_UNSAFE,
		FL_SAFE,
		FL_REMEMBERX,
		FL_RESETX,
		FL_KEEPORDER,
		FL_VARYORDER,
		FL_ENCL_INNER,
		FL_ENCL_OUTER,
		FL_ENCL_FINAL,
		FL_ENCL_ANY,
		FL_ALLOWCROSS,
		FLAGS_COUNT
	};

	extern UnicodeString keywords[KEYWORD_COUNT];
	extern UnicodeString stringbits[STRINGS_COUNT];
	extern UnicodeString flags[FLAGS_COUNT];

	const size_t CG3_BUFFER_SIZE = 8192;
	const size_t NUM_GBUFFERS = 1;
	extern std::vector< std::vector<UChar> > gbuffers;
	const size_t NUM_CBUFFERS = 2;
	extern std::vector<std::string> cbuffers;
}

#endif
