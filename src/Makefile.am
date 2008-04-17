h_sources = Anchor.h BinaryGrammar.h Cohort.h CompositeTag.h ContextualTest.h \
	    GrammarApplicator.h Grammar.h GrammarWriter.h icu_uoptions.h \
	    IGrammarParser.h options.h Reading.h Recycler.h Rule.h Set.h \
	    SingleWindow.h stdafx.h Strings.h Tag.h TextualParser.h uextras.h \
	    version.h Window.h ApertiumApplicator.h

cpp_sources = Anchor.cpp BinaryGrammar.cpp Cohort.cpp CompositeTag.cpp \
	    ContextualTest.cpp GrammarApplicator_matchSet.cpp \
	    GrammarApplicator_reflow.cpp GrammarApplicator_runContextualTest.cpp \
	    GrammarApplicator_runRules.cpp \
	    Grammar.cpp GrammarWriter.cpp icu_uoptions.cpp Reading.cpp \
	    Recycler.cpp Rule.cpp Set.cpp SingleWindow.cpp Strings.cpp Tag.cpp \
	    TextualParser.cpp uextras.cpp Window.cpp 

bin_PROGRAMS = cg-comp cg-proc vislcg3

vislcg3_SOURCES = main.cpp GrammarApplicator_runGrammar.cpp \
		GrammarApplicator.cpp $(cpp_sources)

cg_comp_SOURCES = cg_comp.cpp GrammarApplicator.cpp GrammarApplicator_runGrammar.cpp \
		ApertiumApplicator.cpp $(cpp_sources)

cg_proc_SOURCES = cg_proc.cpp GrammarApplicator.cpp GrammarApplicator_runGrammar.cpp  \
		ApertiumApplicator.cpp $(cpp_sources)

man_MANS=vislcg3.1 cg-proc.1 cg-comp.1

EXTRA_DIST = $(h_sources) $(man_MANS)
