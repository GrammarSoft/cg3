/*
* Copyright (C) 2007-2017, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_USTREAMBUF_HPP
#define c6d28b7452ec699b_USTREAMBUF_HPP

#include "stdafx.hpp"
#include <streambuf>

namespace CG3 {

class cstreambuf : public std::streambuf {
public:
	using Base = std::streambuf;
	using char_type = typename Base::char_type;
	using int_type = typename Base::int_type;

	cstreambuf(FILE* s)
	  : stream(s)
	{
	}

	// Get
	int_type underflow() {
		return fgetc(stream);
	}

	std::streamsize xsgetn(char_type* s, std::streamsize count) {
		return fread(s, 1, static_cast<size_t>(count), stream);
	}

	// Put
	int_type overflow(int_type ch = Base::traits_type::eof()) {
		if (ch != Base::traits_type::eof()) {
			return fputc(ch, stream);
		}
		return 0;
	}

	std::streamsize xsputn(const char_type* s, std::streamsize count) {
		return fwrite(s, 1, static_cast<size_t>(count), stream);
	}

	int sync() {
		return fflush(stream);
	}

private:
	FILE* stream;
};

class bstreambuf : public std::stringbuf {
public:
	using Base = std::stringbuf;
	using char_type = typename Base::char_type;
	using int_type = typename Base::int_type;

	bstreambuf(std::istream& input, std::string&& b)
	  : Base(std::move(b), std::ios::in | std::ios::binary)
	  , stream(&input)
	{
	}

	int_type underflow() {
		auto c = Base::underflow();
		if (c != Base::traits_type::eof()) {
			return c;
		}
		return stream->get();
	}

private:
	std::istream* stream;
};
}

#endif
