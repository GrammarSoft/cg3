/*
* Copyright (C) 2007-2023, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_INLINES_H
#define c6d28b7452ec699b_INLINES_H

template<typename T>
constexpr inline int8_t SI8(T t) {
	return static_cast<int8_t>(t);
}

template<typename T>
constexpr inline int32_t SI32(T t) {
	return static_cast<int32_t>(t);
}

template<typename T>
constexpr inline int64_t SI64(T t) {
	return static_cast<int64_t>(t);
}

template<typename T>
constexpr inline uint8_t UI8(T t) {
	return static_cast<uint8_t>(t);
}

template<typename T>
constexpr inline uint16_t UI16(T t) {
	return static_cast<uint16_t>(t);
}

template<typename T>
constexpr inline uint32_t UI32(T t) {
	return static_cast<uint32_t>(t);
}

template<typename T>
constexpr inline uint64_t UI64(T t) {
	return static_cast<uint64_t>(t);
}

template<typename T>
constexpr inline double DBL(T t) {
	return static_cast<double>(t);
}

template<typename T>
constexpr inline size_t UIZ(T t) {
	return static_cast<size_t>(t);
}

template<typename T>
constexpr inline void* VOIDP(T t) {
	return static_cast<void*>(t);
}

namespace CG3 {

constexpr double NUMERIC_MIN = DBL(-(1ll << 48ll));
constexpr double NUMERIC_MAX = DBL((1ll << 48ll) - 1);
constexpr uint32_t CG3_HASH_SEED = 705577479u;

/*
	Paul Hsieh's SuperFastHash from http://www.azillionmonkeys.com/qed/hash.html
	This version adapted from online on 2008-12-30
//*/
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
	|| defined(_MSC_VER) || defined(__BORLANDC__) || defined(__TURBOC__)
#define get16bits(d) (*((const uint16_t*) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) (((UI32(((const uint8_t*)(d))[1])) << 8) \
					   +UI32(((const uint8_t*)(d))[0]) )
#endif

inline uint32_t SuperFastHash(const char* data, size_t len = 0, uint32_t hash = CG3_HASH_SEED) {
	if (hash == 0) {
		hash = UI32(len);
	}
	uint32_t tmp;
	uint32_t rem;

	if (len == 0 || data == nullptr) {
		return 0;
	}

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (; len > 0; len--) {
		hash += get16bits(data);
		tmp = (get16bits(data + 2) << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		data += 2 * sizeof(uint16_t);
		hash += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
	case 3:
		hash += get16bits(data);
		hash ^= hash << 16;
		hash ^= data[sizeof(uint16_t)] << 18;
		hash += hash >> 11;
		break;
	case 2:
		hash += get16bits(data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1:
		hash += *data;
		hash ^= hash << 10;
		hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	if (hash == 0 || hash == std::numeric_limits<uint32_t>::max() || hash == std::numeric_limits<uint32_t>::max() - 1) {
		hash = CG3_HASH_SEED;
	}

	return hash;
}

inline uint32_t SuperFastHash(const UChar* data, size_t len = 0, uint32_t hash = CG3_HASH_SEED) {
	if (hash == 0) {
		hash = UI32(len);
	}
	uint32_t tmp;
	uint32_t rem;

	if (len == 0 || data == nullptr) {
		return 0;
	}

	rem = len & 1;
	len >>= 1;

	/* Main loop */
	for (; len > 0; len--) {
		hash += data[0];
		tmp = (data[1] << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		data += 2;
		hash += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
	case 1:
		hash += data[0];
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	if (hash == 0 || hash == std::numeric_limits<uint32_t>::max() || hash == std::numeric_limits<uint32_t>::max() - 1) {
		hash = CG3_HASH_SEED;
	}

	return hash;
}

inline uint32_t hash_value(const UChar* str, uint32_t hash = 0, size_t len = 0) {
	if (hash == 0) {
		hash = CG3_HASH_SEED;
	}
	if (len == 0) {
		len = u_strlen(str);
	}
	return SuperFastHash(str, len, hash);
}

inline uint32_t hash_value(const UString& str, uint32_t h = 0) {
	return hash_value(str.data(), h, str.size());
}

inline uint32_t hash_value(const UStringView& str, uint32_t h = 0) {
	return hash_value(str.data(), h, str.size());
}

inline uint32_t hash_value(const char* str, uint32_t hash = 0, size_t len = 0) {
	if (hash == 0) {
		hash = CG3_HASH_SEED;
	}
	if (len == 0) {
		len = strlen(str);
	}
	return SuperFastHash(str, len, hash);
}

inline uint32_t hash_value(uint32_t c, uint32_t h = CG3_HASH_SEED) {
	if (h == 0) {
		h = CG3_HASH_SEED;
	}
	//*
	h = c + (h << 6U) + (h << 16U) - h;
	if (h == 0 || h == std::numeric_limits<uint32_t>::max() || h == std::numeric_limits<uint32_t>::max() - 1) {
		h = CG3_HASH_SEED;
	}
	return h;
	/*/
	uint32_t tmp = SuperFastHash(reinterpret_cast<const char*>(&c), sizeof(c), h);
	return tmp;
	//*/
}

inline uint32_t hash_value(uint64_t c) {
	/*
	uint32_t tmp = hash_value(UI32(c & 0xFFFFFFFF));
	tmp = hash_value(UI32((c >> 32) & 0xFFFFFFFF), tmp);
	return tmp;
	/*/
	uint32_t tmp = SuperFastHash(reinterpret_cast<const char*>(&c), sizeof(c));
	return tmp;
	//*/
}

struct hash_ustring {
	size_t operator()(const UString& str) const {
		return hash_value(str);
	}

	size_t operator()(const UStringView& str) const {
		return hash_value(str);
	}
};

inline UStringView USV(UnicodeString& str) {
	return UStringView(str.getTerminatedBuffer(), str.length());
}

inline UStringView USV(UString& str) {
	return UStringView(str);
}

inline bool ISDELIM(const UChar c) {
	return c == '(' || c == ')' || c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '%' || c == '=';
}

inline bool ISSPACE(const UChar c) {
	if (c <= 0xFF && c != 0x09 && c != 0x0A && c != 0x0D && c != 0x20 && c != 0xA0) {
		return false;
	}
	return (c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D || c == 0xA0 || u_isWhitespace(c));
}

template<typename Char>
inline bool ISSTRING(const Char* p, const uint32_t c) {
	if (p[-1] == '"' && p[c + 1] == '"') {
		return true;
	}
	if (p[-1] == '<' && p[c + 1] == '>') {
		return true;
	}
	return false;
}

inline bool ISNL(const UChar c) {
	return (
	  c == 0x2028L    // Unicode Line Seperator
	  || c == 0x2029L // Unicode Paragraph Seperator
	  || c == 0x000CL // Form Feed
	  || c == 0x000BL // Vertical Tab
	  || c == 0x000AL // ASCII \n
	);
}

template<typename Char>
inline bool ISESC(const Char* p) {
	uint32_t a = 1;
	while (*(p - a) == '\\') {
		a++;
	}
	return (a % 2 == 0);
}

template<typename Char>
inline bool ISSPACE(const Char* p) {
	return ISSPACE(*p) && !ISESC(p);
}

template<typename Char, typename C, size_t N>
inline bool IS_ICASE(const Char* p, const C (&uc)[N], const C (&lc)[N]) {
	// N - 1 due to null terminator for string constants
	if (ISSTRING(p, N - 1)) {
		return false;
	}
	for (size_t i = 0; i < N - 1; ++i) {
		if (p[i] != uc[i] && p[i] != lc[i]) {
			return false;
		}
	}
	return !u_isalnum(p[N - 1]);
}

template<typename Char>
inline void BACKTONL(Char*& p) {
	while (*p && !ISNL(*p) && (*p != ';' || ISESC(p))) {
		p--;
	}
	++p;
}

template<typename Char>
inline uint32_t SKIPLN(Char*& p) {
	while (*p && !ISNL(*p)) {
		++p;
	}
	++p;
	return 1;
}

template<typename Char>
inline uint32_t SKIPWS(Char*& p, const UChar a = 0, const UChar b = 0, const bool allowhash = false) {
	uint32_t s = 0;
	while (*p && *p != a && *p != b) {
		if (ISNL(*p)) {
			++s;
		}
		if (!allowhash && *p == '#' && !ISESC(p)) {
			s += SKIPLN(p);
			p--;
		}
		if (!ISSPACE(*p)) {
			break;
		}
		++p;
	}
	return s;
}

template<typename Char>
inline uint32_t SKIPTOWS(Char*& p, const UChar a = 0, const bool allowhash = false, const bool allowscol = false) {
	uint32_t s = 0;
	while (*p && !ISSPACE(p)) {
		if (!allowhash && *p == '#' && !ISESC(p)) {
			s += SKIPLN(p);
			--p;
		}
		if (ISNL(*p)) {
			++s;
			++p;
		}
		if (!allowscol && *p == ';' && !ISESC(p)) {
			break;
		}
		if (*p == a && !ISESC(p)) {
			break;
		}
		++p;
	}
	return s;
}

template<typename Char>
inline uint32_t SKIPTO(Char*& p, const UChar a) {
	uint32_t s = 0;
	while (*p && (*p != a || ISESC(p))) {
		if (ISNL(*p)) {
			++s;
		}
		++p;
	}
	return s;
}

template<typename Char>
inline void SKIPTO_NOSPAN(Char*& p, const UChar a) {
	while (*p && (*p != a || ISESC(p))) {
		if (ISNL(*p)) {
			break;
		}
		++p;
	}
}

template<typename Char>
inline void SKIPTO_NOSPAN_RAW(Char*& p, const UChar a) {
	while (*p && *p != a) {
		if (ISNL(*p)) {
			break;
		}
		++p;
	}
}

[[noreturn]]
inline void CG3Quit(const int32_t c = 0, const char* file = nullptr, const uint32_t line = 0) {
	if (file && line) {
		std::cerr << std::flush;
		std::cerr << "CG3Quit triggered from " << file << " line " << line << "." << std::endl;
	}
	exit(c);
}

inline constexpr uint64_t make_64(uint32_t hi, uint32_t low) {
	return (UI64(hi) << 32) | UI64(low);
}

template<typename T, size_t N>
inline constexpr size_t size(T (&)[N]) {
	return N;
}

// Older g++ apparently don't check for empty themselves, so we have to.
template<typename C>
inline void clear(C& c) {
	if (!c.empty()) {
		c.clear();
	}
}

template<typename S>
inline bool is_textual(const S& s) {
	return (s.front() == '"' && s.back() == '"') || (s.front() == '<' && s.back() == '>');
}

template<typename S>
inline bool is_internal(const S& s) {
	return (s[0] == '_' && s[1] == 'G' && s[2] == '_');
}

template<typename S>
inline bool is_cg3b(const S& s) {
	return (s[0] == 'C' && s[1] == 'G' && s[2] == '3' && s[3] == 'B');
}

template<typename Cont, typename VT>
inline bool index_matches(const Cont& index, const VT& entry) {
	return (index.find(entry) != index.end());
}

inline void insert_if_exists(boost::dynamic_bitset<>& cont, const boost::dynamic_bitset<>* other) {
	if (other && !other->empty()) {
		cont.resize(std::max(cont.size(), other->size()));
		cont |= *other;
	}
}

template<typename S, typename T>
inline void writeRaw(S& stream, const T& value) {
	stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename S, typename T>
inline void readRaw(S& stream, T& value) {
	stream.read(reinterpret_cast<char*>(&value), sizeof(T));
}

inline void writeUTF8String(std::ostream& output, const UChar* str, size_t len = 0) {
	if (len == 0) {
		len = u_strlen(str);
	}

	std::vector<char> buffer(len * 4);
	int32_t olen = 0;
	UErrorCode status = U_ZERO_ERROR;
	u_strToUTF8(&buffer[0], SI32(len * 4 - 1), &olen, str, SI32(len), &status);

	auto cs = UI16(olen);
	writeRaw(output, cs);
	output.write(&buffer[0], cs);
}

inline void writeUTF8String(std::ostream& output, const UString& str) {
	writeUTF8String(output, str.data(), str.size());
}

template<typename S>
inline UString readUTF8String(S& input) {
	uint16_t len = 0;
	readRaw(input, len);

	UString rv(len, 0);
	std::vector<char> buffer(len);
	input.read(&buffer[0], len);

	int32_t olen = 0;
	UErrorCode status = U_ZERO_ERROR;
	u_strFromUTF8(&rv[0], len, &olen, &buffer[0], len, &status);

	rv.resize(olen);

	return rv;
}

#ifdef _MSC_VER
	// warning C4127: conditional expression is constant
	#pragma warning (disable: 4127)
#endif

// macOS has macros for these that somehow only reveal themselves in the Python binding
#ifdef htonl
#undef htonl
#endif

#ifdef htons
#undef htons
#endif

#ifdef ntohl
#undef ntohl
#endif

#ifdef ntohs
#undef ntohs
#endif

template<typename T>
T htonl(T t) {
	return be::native_to_big(t);
}

template<typename T>
T htons(T t) {
	return be::native_to_big(t);
}

template<typename T>
T ntohl(T t) {
	return be::big_to_native(t);
}

template<typename T>
T ntohs(T t) {
	return be::big_to_native(t);
}

inline uint32_t hton32(uint32_t val) {
	return UI32(htonl(val));
}

inline uint32_t ntoh32(uint32_t val) {
	return UI32(ntohl(val));
}

template<typename T>
inline void writeSwapped(std::ostream& stream, const T& value) {
	if (sizeof(T) == 1) {
		stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}
	else if (sizeof(T) == 2) {
		auto tmp = UI16(htons(UI16(value)));
		stream.write(reinterpret_cast<const char*>(&tmp), sizeof(T));
	}
	else if (sizeof(T) == 4) {
		auto tmp = UI32(htonl(UI32(value)));
		stream.write(reinterpret_cast<const char*>(&tmp), sizeof(T));
	}
	else if (sizeof(T) == 8) {
		uint64_t tmp = value;
#ifndef BIG_ENDIAN
		auto high = hton32(UI32(tmp >> 32));
		auto low = hton32(UI32(tmp & 0xFFFFFFFFULL));
		tmp = (UI64(low) << 32) | high;
#endif
		stream.write(reinterpret_cast<const char*>(&tmp), sizeof(T));
	}
	else {
		throw std::runtime_error("Unhandled type size in writeSwapped()");
	}
	if (!stream) {
		throw std::runtime_error("Stream was in bad state in writeSwapped()");
	}
}

template<>
inline void writeSwapped(std::ostream& stream, const double& value) {
	int exp = 0;
	auto mant64 = UI64(SI64(DBL(std::numeric_limits<int64_t>::max()) * frexp(value, &exp)));
	auto exp32 = UI32(exp);
	writeSwapped(stream, mant64);
	writeSwapped(stream, exp32);
}

template<typename T>
inline T readSwapped(std::istream& stream) {
	if (!stream) {
		throw std::runtime_error("Stream was in bad state in readSwapped()");
	}
	if (sizeof(T) == 1) {
		uint8_t tmp = 0;
		stream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
		return static_cast<T>(tmp);
	}
	if (sizeof(T) == 2) {
		uint16_t tmp = 0;
		stream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
		return static_cast<T>(ntohs(tmp));
	}
	if (sizeof(T) == 4) {
		uint32_t tmp = 0;
		stream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
		return static_cast<T>(ntohl(tmp));
	}
	if (sizeof(T) == 8) {
		uint64_t tmp = 0;
		stream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
#ifndef BIG_ENDIAN
		auto high = ntoh32(UI32(tmp >> 32));
		auto low = ntoh32(UI32(tmp & 0xFFFFFFFFULL));
		tmp = (UI64(low) << 32) | high;
#endif
		return static_cast<T>(tmp);
	}
	throw std::runtime_error("Unhandled type size in readSwapped()");
}

template<>
inline double readSwapped(std::istream& stream) {
	auto mant64 = readSwapped<uint64_t>(stream);
	auto exp = static_cast<int>(readSwapped<int32_t>(stream));

	auto value = DBL(SI64(mant64)) / DBL(std::numeric_limits<int64_t>::max());

	return ldexp(value, exp);
}

#ifdef _MSC_VER
	// warning C4127: conditional expression is constant
	#pragma warning (default: 4127)
#endif

template<typename Cont>
inline void GAppSetOpts_ranged(const char* value, Cont& cont, bool fill = true) {
	cont.clear();
	bool had_range = false;

	auto comma = value;
	do {
		uint32_t low = abs(atoi(comma)), high = low;
		auto delim = strchr(comma, '-');
		auto nextc = strchr(comma, ',');
		if (delim && (nextc == 0 || nextc > delim)) {
			had_range = true;
			high = abs(atoi(delim + 1));
		}
		for (; low <= high; ++low) {
			cont.push_back(low);
		}
	} while ((comma = strchr(comma, ',')) != 0 && ++comma && *comma != 0);

	if (cont.size() == 1 && !had_range && fill) {
		uint32_t val = cont.front();
		cont.clear();
		for (uint32_t i = 1; i <= val; ++i) {
			cont.push_back(i);
		}
	}
}

template<typename T>
class swapper {
public:
	swapper(bool cond, T& a, T& b)
	  : cond(cond)
	  , a(a)
	  , b(b)
	{
		if (cond) {
			std::swap(a, b);
		}
	}

	~swapper() {
		if (cond) {
			std::swap(a, b);
		}
	}

private:
	bool cond;
	T& a;
	T& b;
};

class swapper_false {
public:
	swapper_false(bool cond, bool& b)
	  : val(false)
	  , swp(cond, val, b)
	{}

private:
	bool val;
	swapper<bool> swp;
};

template<typename T>
class uncond_swap {
public:
	uncond_swap(T& a, T b)
	  : a_(a)
	  , b_(b)
	{
		std::swap(a_, b_);
	}

	~uncond_swap() {
		std::swap(a_, b_);
	}

private:
	T& a_;
	T b_;
};

template<typename T>
class inc_dec {
public:
	inc_dec()
	  : p(nullptr)
	{}

	~inc_dec() {
		if (p) {
			--(*p);
		}
	}

	void inc(T& pt) {
		p = &pt;
		++(*p);
	}

private:
	T* p;
};

template<typename T>
inline T* reverse(T* head) {
	T* nr = nullptr;
	while (head) {
		T* next = head->next;
		head->next = nr;
		nr = head;
		head = next;
	}
	return nr;
}

template <typename T>
struct Reversed {
	T& t;
};

template <typename T>
auto begin(Reversed<T> c) {
	return std::rbegin(c.t);
}

template <typename T>
auto end(Reversed<T> c) {
	return std::rend(c.t);
}

template <typename T>
Reversed<T> reversed(T&& c) {
	return { c };
}

template<typename Cont, typename T>
inline void erase(Cont& cont, const T& val) {
	cont.erase(std::remove(cont.begin(), cont.end(), val), cont.end());
}

inline size_t fread_throw(void* buffer, size_t size, size_t count, FILE* stream) {
	size_t rv = ::fread(buffer, size, count, stream);
	if (rv != count) {
		throw std::runtime_error("fread() did not read all requested objects");
	}
	return rv;
}

inline size_t fread_throw(void* buffer, size_t size, size_t count, std::istream& stream) {
	if (!stream.read(static_cast<char*>(buffer), size * count)) {
		throw std::runtime_error("stream did not read all requested objects");
	}
	return size * count;
}

inline size_t fwrite_throw(const void* buffer, size_t size, size_t count, FILE* stream) {
	size_t rv = ::fwrite(buffer, size, count, stream);
	if (rv != count) {
		throw std::runtime_error("fwrite() did not write all requested objects");
	}
	return rv;
}

template<class Function, std::size_t... Indices>
constexpr auto make_array_helper(Function f, std::index_sequence<Indices...>) -> std::array<typename std::invoke_result<Function, std::size_t>::type, sizeof...(Indices)> {
	return { { f(Indices)... } };
}

template<int N, class Function>
constexpr auto make_array(Function f) -> std::array<typename std::invoke_result<Function, std::size_t>::type, N> {
	return make_array_helper(f, std::make_index_sequence<N>{});
}

namespace details {
	inline void _concat(std::string&) {
	}

	template<typename T, typename... Args>
	inline void _concat(std::string& msg, const T& t, Args... args) {
		msg.append(t);
		_concat(msg, args...);
	}
}

template<typename T, typename... Args>
inline std::string concat(const T& value, Args... args) {
	std::string msg(value);
	details::_concat(msg, args...);
	return msg;
}

}

#endif
