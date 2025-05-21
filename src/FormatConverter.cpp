/*
* Copyright (C) 2007-2025, GrammarSoft ApS
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

#include "FormatConverter.hpp"
#include "streambuf.hpp"

namespace CG3 {

constexpr auto BUF_SIZE = 1000;

cg3_sformat detectFormat(std::string_view buf8) {
	cg3_sformat fmt = CG3SF_INVALID;
	UErrorCode status = U_ZERO_ERROR;

	UString buffer(BUF_SIZE, 0);
	int32_t nr = 0;
	u_strFromUTF8(&buffer[0], BUF_SIZE, &nr, buf8.data(), SI32(buf8.size()), &status);
	if (U_FAILURE(status)) {
		throw std::runtime_error("UTF-8 to UTF-16 conversion failed");
	}
	buffer.resize(nr);
	URegularExpression* rx = nullptr;

	for (;;) {
		rx = uregex_openC("^\"<[^>]+>\".*?^\\s+\"[^\"]+\"", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
		uregex_setText(rx, buffer.data(), SI32(buffer.size()), &status);
		if (uregex_find(rx, -1, &status)) {
			fmt = CG3SF_CG;
			break;
		}
		uregex_close(rx);

		rx = uregex_openC("^\\S+ *\t *\\[\\S+\\]", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
		uregex_setText(rx, buffer.data(), SI32(buffer.size()), &status);
		if (uregex_find(rx, -1, &status)) {
			fmt = CG3SF_NICELINE;
			break;
		}
		uregex_close(rx);

		rx = uregex_openC("^\\S+ *\t *\"\\S+\"", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
		uregex_setText(rx, buffer.data(), SI32(buffer.size()), &status);
		if (uregex_find(rx, -1, &status)) {
			fmt = CG3SF_NICELINE;
			break;
		}
		uregex_close(rx);

		rx = uregex_openC("\\^[^/]+(/[^<]+(<[^>]+>)+)+\\$", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
		uregex_setText(rx, buffer.data(), SI32(buffer.size()), &status);
		if (uregex_find(rx, -1, &status)) {
			fmt = CG3SF_APERTIUM;
			break;
		}
		uregex_close(rx);

		rx = uregex_openC("^\\S+\t\\S+(\\+\\S+)+$", UREGEX_DOTALL | UREGEX_MULTILINE, 0, &status);
		uregex_setText(rx, buffer.data(), SI32(buffer.size()), &status);
		if (uregex_find(rx, -1, &status)) {
			fmt = CG3SF_FST;
			break;
		}
		uregex_close(rx);

		rx = uregex_openC("^\\{", UREGEX_MULTILINE, 0, &status);
		uregex_setText(rx, buffer.data(), SI32(buffer.size()), &status);
		if (uregex_find(rx, -1, &status)) {
			fmt = CG3SF_JSONL;
			break;
		}

		fmt = CG3SF_PLAIN;
		break;
	}
	uregex_close(rx);

	return fmt;
}

FormatConverter::FormatConverter(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
  , ApertiumApplicator(ux_err)
  , FSTApplicator(ux_err)
  , JsonlApplicator(ux_err)
  , MatxinApplicator(ux_err)
  , NicelineApplicator(ux_err)
  , PlaintextApplicator(ux_err)
{
	conv_grammar.ux_stderr = &ux_err;
	conv_grammar.allocateDummySet();
	conv_grammar.delimiters = conv_grammar.allocateSet();
	conv_grammar.addTagToSet(conv_grammar.allocateTag(STR_DUMMY), conv_grammar.delimiters);
	conv_grammar.reindex();

	setGrammar(&conv_grammar);
}

std::unique_ptr<std::istream> FormatConverter::detectFormat(std::istream& in) {
	std::unique_ptr<std::istream> instream;

	std::string buf8 = read_utf8(in, BUF_SIZE);

	fmt_input = CG3::detectFormat(buf8);

	instream.reset(new std::istream(new bstreambuf(in, std::move(buf8))));
	return instream;
}

void FormatConverter::runGrammarOnText(std::istream& input, std::ostream& output) {
	ux_stdin = &input;
	ux_stdout = &output;

	switch (fmt_input) {
	case CG3SF_CG: {
		GrammarApplicator::runGrammarOnText(input, output);
		break;
	}
	case CG3SF_APERTIUM: {
		ApertiumApplicator::runGrammarOnText(input, output);
		break;
	}
	case CG3SF_NICELINE: {
		NicelineApplicator::runGrammarOnText(input, output);
		break;
	}
	case CG3SF_PLAIN: {
		PlaintextApplicator::runGrammarOnText(input, output);
		break;
	}
	case CG3SF_FST: {
		FSTApplicator::runGrammarOnText(input, output);
		break;
	}
	case CG3SF_JSONL: {
		JsonlApplicator::runGrammarOnText(input, output);
		break;
	}
	default:
		CG3Quit();
	}
}

void FormatConverter::printCohort(Cohort* cohort, std::ostream& output, bool profiling) {
	switch (fmt_output) {
	case CG3SF_CG: {
		GrammarApplicator::printCohort(cohort, output, profiling);
		break;
	}
	case CG3SF_APERTIUM: {
		ApertiumApplicator::printCohort(cohort, output, profiling);
		break;
	}
	case CG3SF_FST: {
		FSTApplicator::printCohort(cohort, output, profiling);
		break;
	}
	case CG3SF_NICELINE: {
		NicelineApplicator::printCohort(cohort, output, profiling);
		break;
	}
	case CG3SF_PLAIN: {
		PlaintextApplicator::printCohort(cohort, output, profiling);
		break;
	}
	case CG3SF_JSONL: {
		JsonlApplicator::printCohort(cohort, output, profiling);
		break;
	}
	default:
		CG3Quit();
	}
}

void FormatConverter::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
	switch (fmt_output) {
	case CG3SF_CG: {
		GrammarApplicator::printSingleWindow(window, output, profiling);
		break;
	}
	case CG3SF_APERTIUM: {
		ApertiumApplicator::printSingleWindow(window, output, profiling);
		break;
	}
	case CG3SF_FST: {
		FSTApplicator::printSingleWindow(window, output, profiling);
		break;
	}
	case CG3SF_NICELINE: {
		NicelineApplicator::printSingleWindow(window, output, profiling);
		break;
	}
	case CG3SF_PLAIN: {
		PlaintextApplicator::printSingleWindow(window, output, profiling);
		break;
	}
	case CG3SF_JSONL: {
		JsonlApplicator::printSingleWindow(window, output, profiling);
		break;
	}
	default:
		CG3Quit();
}
}

void FormatConverter::printStreamCommand(UStringView cmd, std::ostream& output) {
	switch (fmt_output) {
	case CG3SF_JSONL: {
		JsonlApplicator::printStreamCommand(cmd, output);
		break;
	}
	case CG3SF_CG:
	case CG3SF_APERTIUM:
	case CG3SF_FST:
	case CG3SF_NICELINE:
	case CG3SF_PLAIN:
	default: {
		GrammarApplicator::printStreamCommand(cmd, output);
		break;
	}
	}
}

void FormatConverter::printPlainTextLine(UStringView line, std::ostream& output) {
	switch (fmt_output) {
	case CG3SF_JSONL: {
		JsonlApplicator::printPlainTextLine(line, output);
		break;
	}
	case CG3SF_CG:
	case CG3SF_APERTIUM:
	case CG3SF_FST:
	case CG3SF_NICELINE:
	case CG3SF_PLAIN:
	default: {
		GrammarApplicator::printPlainTextLine(line, output);
		break;
	}
	}
}

}
