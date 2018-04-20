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
#ifndef c6d28b7452ec699b_VERSION_H
#define c6d28b7452ec699b_VERSION_H

#include <cstdint>

constexpr auto CG3_COPYRIGHT_STRING = "Copyright (C) 2007-2018 GrammarSoft ApS. Licensed under GPLv3+";

constexpr uint32_t CG3_VERSION_MAJOR = 1;
constexpr uint32_t CG3_VERSION_MINOR = 1;
constexpr uint32_t CG3_VERSION_PATCH = 4;
constexpr uint32_t CG3_REVISION = 12795;
constexpr uint32_t CG3_FEATURE_REV = 12795;
constexpr uint32_t CG3_TOO_OLD = 10373;
constexpr uint32_t CG3_EXTERNAL_PROTOCOL = 7226;

#endif
