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

#include "MweSplitApplicator.hpp"

namespace CG3 {

MweSplitApplicator::MweSplitApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
{
	CG3::Grammar* grammar = new CG3::Grammar;
	grammar->ux_stderr = ux_stderr;
	grammar->allocateDummySet();
	grammar->delimiters = grammar->allocateSet();
	grammar->addTagToSet(grammar->allocateTag(CG3::stringbits[0].getTerminatedBuffer()), grammar->delimiters);
	grammar->reindex();
	setGrammar(grammar);
	owns_grammar = true;
	is_conv = true;
}

void MweSplitApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
	GrammarApplicator::runGrammarOnText(input, output);
}

const Tag* MweSplitApplicator::maybeWfTag(const Reading* r) {
	for (auto tter : r->tags_list) {
		if ((!show_end_tags && tter == endtag) || tter == begintag) {
			continue;
		}
		if (tter == r->baseform || tter == r->parent->wordform->hash) {
			continue;
		}
		const Tag* tag = single_tags[tter];
		// If we are to split, there has to be at least one wordform on a head (not-sub) reading
		if (tag->type & T_WORDFORM) {
			return tag;
		}
	}
	return NULL;
}

std::vector<Cohort*> MweSplitApplicator::splitMwe(Cohort* cohort) {
	constexpr UChar rtrimblank[] = { ' ', '\n', '\r', '\t', 0 };
	constexpr UChar textprefix[] = { ':', 0 };
	std::vector<Cohort*> cos;
	size_t n_wftags = 0;
	size_t n_goodreadings = 0;
	for (auto rter1 : cohort->readings) {
		if (maybeWfTag(rter1) != NULL) {
			++n_wftags;
		}
		++n_goodreadings;
	}

	if (n_wftags < n_goodreadings) {
		if (n_wftags > 0) {
			u_fprintf(ux_stderr, "WARNING: Line %u: Some but not all main-readings of %S had wordform-tags (not completely mwe-disambiguated?), not splitting.\n", numLines, cohort->wordform->tag.c_str());
			// We also don't split if wordform-tags were only on sub-readings, but should we warn on such faulty input?
		}
		cos.push_back(cohort);
		return cos;
	}
	for (auto r : cohort->readings) {
		size_t pos = std::numeric_limits<size_t>::max();
		Reading* prev = NULL; // prev == NULL || prev->next == rNew (or a ->next of rNew)
		for (auto sub = r; sub; sub = sub->next) {
			const Tag* wfTag = maybeWfTag(sub);
			if (wfTag == NULL) {
				prev = prev->next;
			}
			else {
				++pos;
				Cohort* c;
				while (cos.size() < pos + 1) {
					c = alloc_cohort(cohort->parent);
					c->global_number = gWindow->cohort_counter++;
					cohort->parent->appendCohort(c);
					cos.push_back(c);
				}
				c = cos[pos];

				const size_t wfEnd = wfTag->tag.size() - 3; // index before the final '>"'
				const size_t i = 1 + wfTag->tag.find_last_not_of(rtrimblank, wfEnd);
				const UString& wf = wfTag->tag.substr(0, i) + wfTag->tag.substr(wfEnd + 1);
				if (c->wordform != 0 && wf != c->wordform->tag) {
					u_fprintf(ux_stderr, "WARNING: Line %u: Ambiguous wordform-tags for same cohort, '%S' vs '%S', not splitting.\n", numLines, wf.c_str(), c->wordform->tag.c_str());
					cos.clear();
					cos.push_back(cohort);
					return cos;
				}
				c->wordform = addTag(wf);
				if (i < wfEnd + 1) {
					c->text = textprefix + wfTag->tag.substr(i, wfEnd + 1 - i);
				}

				Reading* rNew = alloc_reading(*sub);
				for (size_t i = 0; i < rNew->tags_list.size(); ++i) {
					auto& tter = rNew->tags_list[i];
					if (tter == wfTag->hash || tter == rNew->parent->wordform->hash) {
						rNew->tags_list.erase(rNew->tags_list.begin() + i);
						rNew->tags.erase(tter);
					}
				}
				cos[pos]->appendReading(rNew);
				rNew->parent = cos[pos];

				if (prev != NULL) {
					free_reading(prev->next);
					prev->next = 0;
				}
				prev = rNew;
			}
		}
	}
	if (cos.size() == 0) {
		u_fprintf(ux_stderr, "WARNING: Line %u: Tried splitting %S, but got no new cohorts; shouldn't happen.", numLines, cohort->wordform->tag.c_str());
		cos.push_back(cohort);
	}
	// The last word forms are the top readings:
	cos[0]->text = cohort->text;
	std::reverse(cos.begin(), cos.end());
	return cos;
}

void MweSplitApplicator::printSingleWindow(SingleWindow* window, std::ostream& output) {
	for (auto var : window->variables_output) {
		Tag* key = single_tags[var];
		auto iter = window->variables_set.find(var);
		if (iter != window->variables_set.end()) {
			if (iter->second != grammar->tag_any) {
				Tag* value = single_tags[iter->second];
				u_fprintf(output, "%S%S=%S>\n", stringbits[S_CMD_SETVAR].getTerminatedBuffer(), key->tag.c_str(), value->tag.c_str());
			}
			else {
				u_fprintf(output, "%S%S>\n", stringbits[S_CMD_SETVAR].getTerminatedBuffer(), key->tag.c_str());
			}
		}
		else {
			u_fprintf(output, "%S%S>\n", stringbits[S_CMD_REMVAR].getTerminatedBuffer(), key->tag.c_str());
		}
	}

	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.c_str());
		if (!ISNL(window->text[window->text.length() - 1])) {
			u_fputc('\n', output);
		}
	}

	uint32_t cs = (uint32_t)window->cohorts.size();
	for (uint32_t c = 0; c < cs; c++) {
		Cohort* cohort = window->cohorts[c];
		std::vector<Cohort*> cs = splitMwe(cohort);
		for (auto iter : cs) {
			printCohort(iter, output);
		}
	}
	u_fputc('\n', output);
	u_fflush(output);
}
}
