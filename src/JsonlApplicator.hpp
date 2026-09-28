/*
* Copyright (C) 2025-2026, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* JSONL I/O developed by Robert Reynolds <reynoldsrjr@gmail.com>
* Based on contributions from GitHub Copilot
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

#ifndef c6d28b7452ec699b_JSONLAPPLICATOR_HPP
#define c6d28b7452ec699b_JSONLAPPLICATOR_HPP

#include "GrammarApplicator.hpp"
#include <rapidjson/document.h>

namespace CG3 {

/*
* JSONL stream format: one JSON object per line. Blank lines are skipped, and lines that are not a JSON
* object are skipped with a warning. The full specification is in manual/streamformats.xml (stream-jsonl).
*
* Cohort:     {"w":"word", "wb":"...", "sts":[...], "z":"...", "ds":1, "dp":0, "id":1, "rels":{"name":[2]},
*              "rs":[reading...], "drs":[reading...]}
* Reading:    {"l":"lemma", "ts":["N","@SUBJ"], "tr":["SELECT:12"], "s":{sub-reading}}
* Text:       {"t":"one line of text"}
* Command:    {"cmd":"<STREAMCMD:FLUSH>"}
*
* "w" has no "<>" quotes and "l" has no "" quotes. Mapping tags go in "ts". "ds"/"dp" are window-local
* numbers with 0 as the root, or global numbers with --dep-absolute or once a dependency has spanned windows.
*/
class JsonlApplicator : public virtual GrammarApplicator {
public:
	JsonlApplicator(std::ostream& ux_err);
	~JsonlApplicator() override;

	void runGrammarOnText(std::istream& input, std::ostream& output) override;

protected:
	void printCohort(Cohort* cohort, std::ostream& output, bool profiling = false) override;
	void printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling = false) override;
	void printStreamCommand(UStringView cmd, std::ostream& output) override;
	void printPlainTextLine(UStringView line, std::ostream& output) override;

private:
	bool getJsonString(const rapidjson::Value& obj, const char* key, UString& out);
	Cohort* parseJsonCohort(const rapidjson::Value& obj, SingleWindow* cSWindow);
	Reading* parseJsonReading(const rapidjson::Value& reading_obj, Cohort* cohort, TagList& mappings);
	void addSingleMapping(Reading& reading, TagList& mappings);
	void buildJsonReading(const Reading* reading, rapidjson::Value& reading_json, rapidjson::Document::AllocatorType& allocator);
	void buildJsonTags(const Reading* reading, rapidjson::Value& tags_json, rapidjson::Document::AllocatorType& allocator);
	void printText(const UString& text, std::ostream& output);
	void printVariables(const uint32SortedVector& vars_output, const uint32FlatHashMap& vars_set, std::ostream& output);
};

}

#endif
