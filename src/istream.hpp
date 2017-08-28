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
#ifndef c6d28b7452ec699b_ISTREAM_HPP
#define c6d28b7452ec699b_ISTREAM_HPP

#include "stdafx.hpp"

namespace CG3 {

class istream {
public:
	istream(UFILE* s, bool strip_bom = true)
	  : stream(s)
	  , raw(u_fgetfile(stream))
	{
		if (strip_bom) {
			UChar32 bom = u_fgetcx(stream);
			if (bom != 0xfeff && bom != static_cast<UChar32>(0xffffffff)) {
				u_fungetc(bom, stream);
			}
		}
	}

	virtual ~istream() {
		u_fclose(stream);
	}

	virtual bool good() {
		return (stream != 0);
	}

	virtual UBool eof() {
		return u_feof(stream);
	}

	virtual UChar* gets(UChar* s, int32_t n) {
		return u_fgets(s, n, stream);
	}

	virtual UChar getc() {
		return u_fgetc(stream);
	}

	virtual int getc_raw() {
		return fgetc(raw);
	}

private:
	UFILE* stream;
	FILE* raw;
};

class istream_buffer : public istream {
public:
	istream_buffer(UFILE* s, const UString& b)
	  : istream(s)
	  , offset(0)
	  , raw_offset(0)
	  , buffer(b)
	{
		buffer.resize(buffer.size() + 1);
		buffer.resize(buffer.size() - 1);
	}

	UBool eof() {
		if (offset >= buffer.size() || raw_offset >= buffer.size() * sizeof(buffer[0])) {
			return istream::eof();
		}
		return false;
	}

	UChar* gets(UChar* s, int32_t m) {
		if (offset < buffer.size()) {
			std::fill(s, s + m, static_cast<UChar>(0));
			UChar* p = &buffer[offset];
			UChar* n = p;
			SKIPLN(n);
			if (n - p > m) {
				n = p + m;
			}
			std::copy(p, n, s);
			size_t len = n - p;
			offset += len;
			if (!ISNL(n[-1])) {
				istream::gets(s + (len - 1), m - len);
			}
			return s;
		}
		return istream::gets(s, m);
	}

	UChar getc() {
		if (offset < buffer.size()) {
			return buffer[offset++];
		}
		return istream::getc();
	}

	int getc_raw() {
		if (raw_offset < buffer.size() * sizeof(buffer[0])) {
			return reinterpret_cast<char*>(&buffer[0])[raw_offset++];
		}
		return istream::getc_raw();
	}

private:
	size_t offset, raw_offset;
	UString buffer;
};
}

#endif
