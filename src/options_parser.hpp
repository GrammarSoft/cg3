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
#ifndef c6d28b7452ec699b_OPTIONS_PARSER_H
#define c6d28b7452ec699b_OPTIONS_PARSER_H

auto options_default = options;
auto options_override = options;

inline void parse_opts(const char* which, decltype(options)& where) {
	if (auto _env = getenv(which)) {
		std::string env(_env);
		std::vector<char*> argv(1); // 0th element is the program name
		auto p = &env[0];
		while (*p) {
			while (*p && CG3::ISSPACE(*p)) {
				++p;
			}
			if (*p == '-') {
				auto n = p;
				CG3::SKIPTOWS(p);
				*p = 0;
				argv.push_back(n);
				++p;
			}
			else if (*p == '"') {
				++p;
				auto n = p;
				CG3::SKIPTO(p, '"');
				*p = 0;
				argv.push_back(n);
				++p;
			}
			else if (*p == '\'') {
				++p;
				auto n = p;
				CG3::SKIPTO(p, '\'');
				*p = 0;
				argv.push_back(n);
				++p;
			}
			else {
				auto n = p;
				CG3::SKIPTOWS(p);
				*p = 0;
				argv.push_back(n);
				++p;
			}
		}
		u_parseArgs(static_cast<int>(argv.size()), &argv[0], NUM_OPTIONS, where.data());
	}
}

#endif
