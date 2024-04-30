/*
* Copyright (C) 2007-2024, GrammarSoft ApS
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
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_MATH_PARSER_HPP
#define c6d28b7452ec699b_MATH_PARSER_HPP

#include <stdexcept>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cmath>

#include "stdafx.hpp"

namespace CG3 {

// Adapted from https://csvparser.github.io/mathparser.html which in turn is adapted from book "C++ The Complete Reference" by H.Schildt.

class MathParser {
	enum type_t:uint8_t { DELIMITER = 1, VARIABLE, NUMBER, FUNCTION };
	constexpr static size_t NUMVARS = 26;

	UStringView exp_ptr;
	UStringView token;
	char tok_type = 0;
	double vars[NUMVARS] = {0};
	double min = 0;
	double max = 0;
	void eval_assign(double& result);
	void eval_add_sub(double& result);
	void eval_mul_div(double& result);
	void eval_exp(double& result);
	void eval_unary(double& result);
	void eval_func(double& result);
	void get_token();

public:
	MathParser(double min=0, double max=0) : min(min), max(max) {}
	double eval(UStringView exp);
};

inline double MathParser::eval(UStringView exp) {
	double result = 0;
	exp_ptr = exp;
	get_token();
	if (token.empty()) {
		throw std::runtime_error("Expression empty");
	}
	eval_assign(result);
	if (!token.empty()) {
		throw std::runtime_error("Syntax error");
	}
	return result;
}

inline void MathParser::eval_assign(double& result) {
	UStringView temp_token;
	if (tok_type == VARIABLE) {
		UStringView t_ptr = exp_ptr;
		temp_token = token;
		auto slot = token[0] - 'A';
		get_token();
		if (token[0] != '=') {
			exp_ptr = t_ptr;
			token = temp_token;
			tok_type = VARIABLE;
		}
		else {
			get_token();
			eval_add_sub(result);
			vars[slot] = result;
			return;
		}
	}
	eval_add_sub(result);
}

inline void MathParser::eval_add_sub(double& result) {
	UChar op = 0;
	double temp = 0;
	eval_mul_div(result);
	while ((op = token[0]) == '+' || op == '-') {
		get_token();
		eval_mul_div(temp);
		switch (op) {
		case '-':
			result = result - temp;
			break;
		case '+':
			result = result + temp;
			break;
		}
	}
}

inline void MathParser::eval_mul_div(double& result) {
	UChar op = 0;
	double temp = 0;
	eval_exp(result);
	while ((op = token[0]) == '*' || op == '/') {
		get_token();
		eval_exp(temp);
		switch (op) {
		case '*':
			result = result * temp;
			break;
		case '/':
			result = result / temp;
			break;
		}
	}
}

inline void MathParser::eval_exp(double& result) {
	double temp = 0;
	eval_unary(result);
	while (token[0] == '^') {
		get_token();
		eval_unary(temp);
		result = pow(result, temp);
	}
}

inline void MathParser::eval_unary(double& result) {
	UChar op = 0;
	if ((tok_type == DELIMITER) && (token[0] == '+' || token[0] == '-')) {
		op = token[0];
		get_token();
	}
	eval_func(result);
	if (op == '-') {
		result = -result;
	}
}

// Process a function, a parenthesized expression, a value or a variable
inline void MathParser::eval_func(double& result) {
	bool isfunc = (tok_type == FUNCTION);
	UStringView temp_token;
	if (isfunc) {
		temp_token = token;
		get_token();
	}
	if (token[0] == '(') {
		get_token();
		eval_add_sub(result);
		if (token[0] != ')') {
			throw std::runtime_error("Unbalanced parentheses");
		}
		if (isfunc) {
			if (ux_simplecasecmp(temp_token, u"SIN")) {
				result = sin(M_PI / 180.0 * result);
			}
			else if (ux_simplecasecmp(temp_token, u"COS")) {
				result = cos(M_PI / 180.0 * result);
			}
			else if (ux_simplecasecmp(temp_token, u"TAN")) {
				result = tan(M_PI / 180.0 * result);
			}
			else if (ux_simplecasecmp(temp_token, u"ASIN")) {
				result = 180.0 / M_PI * asin(result);
			}
			else if (ux_simplecasecmp(temp_token, u"ACOS")) {
				result = 180.0 / M_PI * acos(result);
			}
			else if (ux_simplecasecmp(temp_token, u"ATAN")) {
				result = 180.0 / M_PI * atan(result);
			}
			else if (ux_simplecasecmp(temp_token, u"SINH")) {
				result = sinh(result);
			}
			else if (ux_simplecasecmp(temp_token, u"COSH")) {
				result = cosh(result);
			}
			else if (ux_simplecasecmp(temp_token, u"TANH")) {
				result = tanh(result);
			}
			else if (ux_simplecasecmp(temp_token, u"ASINH")) {
				result = asinh(result);
			}
			else if (ux_simplecasecmp(temp_token, u"ACOSH")) {
				result = acosh(result);
			}
			else if (ux_simplecasecmp(temp_token, u"ATANH")) {
				result = atanh(result);
			}
			else if (ux_simplecasecmp(temp_token, u"LN")) {
				result = log(result);
			}
			else if (ux_simplecasecmp(temp_token, u"LOG")) {
				result = log10(result);
			}
			else if (ux_simplecasecmp(temp_token, u"EXP")) {
				result = exp(result);
			}
			else if (ux_simplecasecmp(temp_token, u"SQRT")) {
				result = sqrt(result);
			}
			else if (ux_simplecasecmp(temp_token, u"SQR")) {
				result = result * result;
			}
			else if (ux_simplecasecmp(temp_token, u"ROUND")) {
				result = round(result);
			}
			else if (ux_simplecasecmp(temp_token, u"FLOOR")) {
				result = floor(result);
			}
			else {
				throw std::runtime_error("Unknown function");
			}
		}
		get_token();
	}
	else {
		switch (tok_type) {
		case VARIABLE:
			if (ux_simplecasecmp(token, u"MIN")) {
				result = min;
			}
			else if (ux_simplecasecmp(token, u"MAX")) {
				result = max;
			}
			else {
				result = vars[token[0] - 'A'];
			}
			get_token();
			return;
		case NUMBER: {
			char num[128];
			std::copy(token.begin(), token.end(), num);
			num[token.size()] = 0;
			result = strtod(num, nullptr);
			if (errno == ERANGE) {
				throw std::runtime_error("Result did not fit in a double");
			}
			get_token();
			return;
		}
		default:
			throw std::runtime_error("Syntax error");
		}
	}
}

inline void MathParser::get_token() {
	token = exp_ptr;
	tok_type = 0;
	if (exp_ptr.empty()) {
		return;
	}
	while (ISSPACE(exp_ptr[0])) {
		exp_ptr.remove_prefix(1);
	}

	if (ISDELIM(exp_ptr[0])) {
		tok_type = DELIMITER;
		token = exp_ptr.substr(0, 1);
		exp_ptr.remove_prefix(1);
	}
	else if (isalpha(exp_ptr[0])) {
		token = exp_ptr.substr(0, exp_ptr.find_first_of(u" +-/*%^=()"));
		exp_ptr.remove_prefix(token.size());
		while (ISSPACE(exp_ptr[0])) {
			exp_ptr.remove_prefix(1);
		}
		tok_type = (exp_ptr[0] == '(') ? FUNCTION : VARIABLE;
	}
	else if (isdigit(exp_ptr[0]) || exp_ptr[0] == '.') {
		token = exp_ptr.substr(0, exp_ptr.find_first_of(u" +-/*%^=()"));
		exp_ptr.remove_prefix(token.size());
		tok_type = NUMBER;
	}

	if ((tok_type == VARIABLE)) {
		if (ux_simplecasecmp(token, u"MIN") || ux_simplecasecmp(token, u"MAX")) {
			// Nothing
		}
		else if (token.size() > 1) {
			throw std::runtime_error("Variables other than MIN and MAX must be 1 letter");
		}
	}
}

}

#endif
