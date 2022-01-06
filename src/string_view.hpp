/*
* Copyright (C) 2007-2021, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_STRING_VIEW_HPP_
#define c6d28b7452ec699b_STRING_VIEW_HPP_

#ifdef HAS_SV
	#include <string_view>
#else
	#include <string>
	#include <experimental/string_view>
	namespace std {
		using string_view = ::std::experimental::string_view;
		template<typename T>
		using basic_string_view = ::std::experimental::basic_string_view<T>;

		inline ::std::string& operator+=(::std::string& str, ::std::string_view sv) {
			str.append(sv.begin(), sv.end());
			return str;
		}
	}
#endif

#endif
