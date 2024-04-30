/*
* Copyright (C) 2007-2024, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_OPTIONS_PARSER_H
#define c6d28b7452ec699b_OPTIONS_PARSER_H

auto options_default = options;
auto options_override = options;

auto grammar_options_default = options;
auto grammar_options_override = options;

inline void parse_opts(char* p, decltype(options)& where) {
	using namespace CG3;
	std::vector<char*> argv(1); // 0th element is the program name
	while (*p) {
		while (*p && ISSPACE(*p)) {
			++p;
		}
		if (*p == '-') {
			auto n = p;
			SKIPTOWS(p);
			*p = 0;
			argv.push_back(n);
			++p;
		}
		else if (*p == '"') {
			++p;
			auto n = p;
			SKIPTO(p, '"');
			*p = 0;
			argv.push_back(n);
			++p;
		}
		else if (*p == '\'') {
			++p;
			auto n = p;
			SKIPTO(p, '\'');
			*p = 0;
			argv.push_back(n);
			++p;
		}
		else {
			auto n = p;
			SKIPTOWS(p);
			*p = 0;
			argv.push_back(n);
			++p;
		}
	}
	u_parseArgs(static_cast<int>(argv.size()), &argv[0], NUM_OPTIONS, where.data());
}

inline void parse_opts_env(const char* which, decltype(options)& where) {
	using namespace CG3;
	if (auto _env = getenv(which)) {
		std::string env(_env);
		env.push_back(0);
		parse_opts(&env[0], where);
	}
}

#endif
