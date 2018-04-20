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
		setg(&ch, &ch + 1, &ch + 1);
	}

	// Get
	int_type underflow() {
		auto c = fgetc(stream);
		ch = static_cast<char_type>(c);
		setg(&ch, &ch, &ch + 1);
		return c;
	}

	std::streamsize xsgetn(char_type* s, std::streamsize count) {
		setg(&ch, &ch + 1, &ch + 1);
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
	char_type ch = 0;
	FILE* stream;
};

class bstreambuf : public std::streambuf {
public:
	using Base = std::streambuf;
	using char_type = typename Base::char_type;
	using int_type = typename Base::int_type;

	bstreambuf(std::istream& input, std::string&& b)
	  : buffer(std::move(b))
	  , stream(&input)
	{
		setg(&ch, &ch + 1, &ch + 1);
	}

	int_type underflow() {
		int_type c = 0;
		if (offset < buffer.size()) {
			c = static_cast<std::make_unsigned<char_type>::type>(buffer[offset++]);
		}
		else {
			c = stream->get();
		}
		ch = static_cast<char_type>(c);
		setg(&ch, &ch, &ch + 1);
		return c;
	}

	std::streamsize xsgetn(char_type* s, std::streamsize count) {
		std::streamsize i = 0;
		for (; offset < buffer.size() && i < count; ++i) {
			s[i] = buffer[offset++];
		}
		if (i < count) {
			stream->read(s + i, count - i);
			i += stream->gcount();
		}
		s[i] = 0;
		setg(&ch, &ch + 1, &ch + 1);
		return i;
	}

private:
	std::string buffer;
	char_type ch = 0;
	size_t offset = 0;
	std::istream* stream;
};
}

#endif
