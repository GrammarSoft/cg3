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
#ifndef c6d28b7452ec699b_GRAMMARAPPLICATORBINARY_H
#define c6d28b7452ec699b_GRAMMARAPPLICATORBINARY_H

#include "GrammarApplicator.hpp"

namespace CG3 {

enum BinaryFormatFlags {
	// Window
	BFW_DEP_SPAN      = (1 << 0),
	// Cohort
	BFC_RELATED       = (1 << 0),
	// Reading
	BFR_SUBREADING    = (1 << 0),
	BFR_DELETED       = (1 << 1),
	// Variables
	BFV_SETVAR        = 1,
	BFV_SETVAR_ANY    = 2,
	BFV_REMVAR        = 3,
};

enum BinaryPacketType : uint8_t {
	BFP_INVALID       = 0,
	BFP_WINDOW        = 1,
	BFP_COMMAND       = 2,
	BFP_TEXT          = 3,
};

enum BinaryCommandType : uint8_t {
	BFC_FLUSH         = 1,
	BFC_EXIT          = 2,
	BFC_IGNORE        = 3,
	BFC_RESUME        = 4,
};

struct BinaryPacket {
	BinaryPacketType type = BFP_INVALID;
	void* payload = nullptr;
};

class BinaryApplicator : public virtual GrammarApplicator {
public:
	BinaryApplicator(std::ostream& ux_err);

	void runGrammarOnText(std::istream& input, std::ostream& output);

protected:
	void printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling = false) override;
	void printStreamCommand(UStringView cmd, std::ostream& output) override;
	void printPlainTextLine(UStringView line, std::ostream& output) override;

private:
	bool header_done = false;
	UString text;
	BinaryPacket readPacket();
	void readWindow(void*& payload);
	void readCommand(void*& payload);
	void readText(void*& payload);
};
}

#endif
