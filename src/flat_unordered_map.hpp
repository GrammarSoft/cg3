/*
* Copyright (C) 2007-2023, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_FLAT_UNORDERED_MAP_HPP
#define c6d28b7452ec699b_FLAT_UNORDERED_MAP_HPP
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <utility>
#include <cstdint>

namespace CG3 {

template<typename T, typename V, T res_empty = T(-1), T res_del = T(-1) - 1>
class flat_unordered_map {
public:
	using value_type = std::pair<const T, V>;
	using value_type_real = std::pair<T, V>;
	using key_type = T;
	using mapped_type = V;
	using container = typename std::vector<value_type_real>;
	using size_type = typename container::size_type;

	class const_iterator {
	private:
		friend class flat_unordered_map;
		const flat_unordered_map* fus;
		size_t i;

	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = flat_unordered_map::value_type;
		using difference_type = ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type;

		const_iterator()
		  : fus(nullptr)
		  , i(0)
		{
		}

		const_iterator(const const_iterator& o)
		  : fus(o.fus)
		  , i(o.i)
		{
		}

		const_iterator(const flat_unordered_map& fus, size_t i = 0)
		  : fus(&fus)
		  , i(i)
		{
		}

		const_iterator& operator=(const const_iterator& o) {
			fus = o.fus;
			i = o.i;
			return *this;
		}

		const_iterator& operator++() {
			for (++i; i < fus->capacity(); ++i) {
				if (fus->elements[i].first != res_empty && fus->elements[i].first != res_del) {
					break;
				}
			}
			if (i >= fus->capacity()) {
				fus = nullptr;
				i = 0;
			}
			return *this;
		}

		const_iterator& operator--() {
			if (i == 0) {
				fus = nullptr;
				i = 0;
			}
			else {
				for (--i; i > 0; --i) {
					if (fus->elements[i].first != res_empty && fus->elements[i].first != res_del) {
						break;
					}
				}
			}
			return *this;
		}

		bool operator==(const const_iterator& o) const {
			return fus == o.fus && i == o.i;
		}

		bool operator!=(const const_iterator& o) const {
			return !(*this == o);
		}

		const value_type_real& operator*() const {
			return fus->elements[i];
		}

		const value_type_real* operator->() const {
			return &fus->elements[i];
		}
	};

	typedef const_iterator iterator;

	enum {
		DEFAULT_CAP = static_cast<size_type>(16u),
	};

	size_t insert(const value_type& t) {
		assert(t.first != res_empty && t.first != res_del && "Key cannot be res_empty or res_del!");

		if (deleted && size_ + deleted == capacity()) {
			reserve(capacity());
		}

		if ((size_ + 1) * 3 / 2 >= capacity() / 2) {
			reserve(std::max(static_cast<size_type>(DEFAULT_CAP), capacity() * 2));
		}
		size_t max = capacity() - 1;
		size_t spot = hash_value(t.first) & max;
		while (elements[spot].first != res_empty && elements[spot].first != t.first) {
			spot = hash_value_sz(spot) & max;
		}
		if (elements[spot].first != t.first) {
			elements[spot] = t;
			++size_;
		}
		return spot;
	}

	template<typename It>
	void insert(It b, It e) {
		size_t d = std::distance(b, e);
		size_t c = capacity();
		while ((size_ + d) * 3 / 2 >= c / 2) {
			c = std::max(static_cast<size_type>(DEFAULT_CAP), c * 2);
		}
		if (c != capacity()) {
			reserve(c);
		}

		for (; b != e; ++b) {
			insert(*b);
		}
	}

	void erase(T t) {
		assert(t != res_empty && t != res_del && "Key cannot be res_empty or res_del!");

		if (size_ == 0) {
			return;
		}
		size_t max = capacity() - 1;
		size_t spot = hash_value(t) & max;
		while (elements[spot].first != res_empty && elements[spot].first != t) {
			spot = hash_value_sz(spot) & max;
		}
		if (elements[spot].first == t) {
			elements[spot].first = res_del;
			elements[spot].second = V();
			--size_;
			if (size_ == 0 && deleted != 0) {
				clear();
			}
			else {
				++deleted;
			}
		}
	}

	const_iterator erase(const_iterator it) {
		elements[it.i].first = res_del;
		elements[it.i].second = V();
		++it;
		--size_;
		if (size_ == 0 && deleted != 0) {
			clear();
		}
		else {
			++deleted;
		}
		return it;
	}

	const_iterator find(T t) const {
		assert(t != res_empty && t != res_del && "Key cannot be res_empty or res_del!");

		const_iterator it;

		if (size_) {
			size_t max = capacity() - 1;
			size_t spot = hash_value(t) & max;
			for (size_t i = 0; i < capacity() * 4 && elements[spot].first != res_empty && elements[spot].first != t; ++i) {
				spot = hash_value_sz(spot) & max;
			}
			if (elements[spot].first == t) {
				it.fus = this;
				it.i = spot;
			}
		}

		return it;
	}

	const_iterator find(T t) {
		if (deleted && size_ + deleted == capacity()) {
			reserve(capacity());
		}

		return const_cast<const flat_unordered_map*>(this)->find(t);
	}

	size_t count(T t) const {
		return (find(t) != end());
	}

	V& operator[](const T& t) {
		assert(t != res_empty && t != res_del && "Key cannot be res_empty or res_del!");

		if (deleted && size_ + deleted == capacity()) {
			reserve(capacity());
		}

		size_t at = std::numeric_limits<size_t>::max();
		if (size_) {
			size_t max = capacity() - 1;
			size_t spot = hash_value(t) & max;
			while (elements[spot].first != res_empty && elements[spot].first != t) {
				spot = hash_value_sz(spot) & max;
			}
			if (elements[spot].first == t) {
				at = spot;
			}
		}
		if (at == std::numeric_limits<size_t>::max()) {
			at = insert(std::make_pair(t, V()));
		}

		return elements[at].second;
	}

	const_iterator begin() const {
		if (size_ == 0) {
			return end();
		}
		for (size_t i = 0, ie = capacity(); i < ie; ++i) {
			if (elements[i].first != res_empty && elements[i].first != res_del) {
				return const_iterator(*this, i);
			}
		}
		return end();
	}

	const_iterator end() const {
		return const_iterator();
	}

	size_type size() const {
		return size_;
	}

	size_type capacity() const {
		return elements.size();
	}

	void reserve(size_type n) {
		if (size_ == 0) {
			elements.resize(n, std::make_pair(res_empty, V()));
			deleted = 0;
			return;
		}

		static container vals;
		vals.resize(0);
		vals.reserve(size_);
		for (auto& elem : elements) {
			if (elem.first != res_empty && elem.first != res_del) {
				vals.push_back(elem);
			}
		}

		clear(n);
		size_ = vals.size();
		size_t max = capacity() - 1;
		for (auto& val : vals) {
			size_t spot = hash_value(val.first) & max;
			while (elements[spot].first != res_empty && elements[spot].first != val.first) {
				spot = hash_value_sz(spot) & max;
			}
			elements[spot] = val;
		}
	}

	bool empty() const {
		return (size_ == 0);
	}

	template<typename It>
	void assign(It b, It e) {
		clear();
		insert(b, e);
	}

	void swap(flat_unordered_map& other) {
		std::swap(size_, other.size_);
		std::swap(deleted, other.deleted);
		elements.swap(other.elements);
	}

	void clear(size_type n = 0) {
		size_ = elements.size();
		elements.resize(0);
		elements.resize(std::max(size_, n), std::make_pair(res_empty, V()));
		size_ = 0;
		deleted = 0;
	}

	container& get() {
		return elements;
	}

private:
	size_type size_ = 0;
	size_type deleted = 0;
	container elements;

	size_type hash_value_sz(size_type t) const {
		return t * 3663850746527583589ull + 11210403176660999867ull;
	}

	size_type hash_value(T t) const {
		return hash_value_sz(static_cast<size_type>(t));
	}

	friend class const_iterator;
};

typedef flat_unordered_map<uint32_t, uint32_t> uint32FlatHashMap;
}

#endif
