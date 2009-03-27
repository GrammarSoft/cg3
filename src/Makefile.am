h_sources = Anchor.h BinaryGrammar.h Cohort.h CompositeTag.h ContextualTest.h \
	    GrammarApplicator.h Grammar.h GrammarWriter.h icu_uoptions.h \
	    IGrammarParser.h options.h Reading.h Recycler.h Rule.h Set.h \
	    SingleWindow.h stdafx.h Strings.h Tag.h TextualParser.h uextras.h \
	    version.h Window.h ApertiumApplicator.h inlines.h macros.h cycle.h

cpp_shared_sources = Anchor.cpp BinaryGrammar.cpp Cohort.cpp CompositeTag.cpp \
	    ContextualTest.cpp Grammar.cpp Reading.cpp Recycler.cpp Rule.cpp \
		Set.cpp Strings.cpp Tag.cpp TextualParser.cpp uextras.cpp \
	    BinaryGrammar_read.cpp BinaryGrammar_write.cpp

cpp_sources = GrammarApplicator_runGrammar.cpp \
		GrammarApplicator.cpp \
	    GrammarApplicator_matchSet.cpp \
	    GrammarApplicator_reflow.cpp GrammarApplicator_runContextualTest.cpp \
	    GrammarApplicator_runRules.cpp \
	    GrammarWriter.cpp icu_uoptions.cpp \
	    SingleWindow.cpp \
	    Window.cpp \
		$(cpp_shared_sources)

bin_PROGRAMS = cg-comp cg-proc vislcg3

vislcg3_SOURCES = main.cpp \
		$(cpp_sources)

cg_comp_SOURCES = cg_comp.cpp \
		$(cpp_shared_sources)

cg_proc_SOURCES = cg_proc.cpp ApertiumApplicator.cpp \
		$(cpp_sources)

noinst_PROGRAMS = test_hashcol_dep test_hashcol_deptag test_hash_arch

test_hashcol_deptag_SOURCES = test_hashcol_deptag.cpp Tag.cpp Strings.cpp uextras.cpp

test_hashcol_dep_SOURCES = test_hashcol_dep.cpp Tag.cpp Strings.cpp uextras.cpp

test_hash_arch_SOURCES = test_hash_arch.cpp

man_MANS=vislcg3.1 cg-proc.1 cg-comp.1

EXTRA_DIST = $(h_sources) $(man_MANS)
