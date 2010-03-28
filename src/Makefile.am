h_sources = Anchor.h BinaryGrammar.h Cohort.h CompositeTag.h ContextualTest.h \
	    GrammarApplicator.h Grammar.h GrammarWriter.h icu_uoptions.h \
	    IGrammarParser.h options.h Reading.h Rule.h Set.h CohortIterator.h \
	    SingleWindow.h stdafx.h Strings.h Tag.h TextualParser.h uextras.h \
	    version.h Window.h ApertiumApplicator.h inlines.h macros.h cycle.h MatxinApplicator.h \
		FormatConverter.h sorted_vector.hpp bloomish.hpp

cpp_shared_sources = Anchor.cpp BinaryGrammar.cpp CompositeTag.cpp \
	    ContextualTest.cpp Grammar.cpp Rule.cpp \
		Set.cpp Strings.cpp Tag.cpp TextualParser.cpp uextras.cpp \
	    BinaryGrammar_read.cpp BinaryGrammar_write.cpp

cpp_sources = GrammarApplicator_runGrammar.cpp \
		GrammarApplicator.cpp \
	    GrammarApplicator_matchSet.cpp \
	    GrammarApplicator_reflow.cpp GrammarApplicator_runContextualTest.cpp \
	    GrammarApplicator_runRules.cpp \
	    GrammarWriter.cpp icu_uoptions.cpp \
	    SingleWindow.cpp CohortIterator.cpp \
	    Window.cpp Cohort.cpp Reading.cpp \
		$(cpp_shared_sources)

bin_PROGRAMS = cg-comp cg-proc cg-conv vislcg3

vislcg3_SOURCES = main.cpp \
		$(cpp_sources)

cg_comp_SOURCES = cg_comp.cpp \
		$(cpp_shared_sources)

cg_proc_SOURCES = cg_proc.cpp ApertiumApplicator.cpp MatxinApplicator.cpp \
		$(cpp_sources)

cg_conv_SOURCES = cg_conv.cpp FormatConverter.cpp ApertiumApplicator.cpp MatxinApplicator.cpp \
		$(cpp_sources)

man_MANS=vislcg3.1 cg-proc.1 cg-comp.1

EXTRA_DIST = $(h_sources) $(man_MANS)
