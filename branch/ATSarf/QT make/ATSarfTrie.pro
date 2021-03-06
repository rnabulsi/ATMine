TOP = ../third/
INCLUDEPATH += $$QTDIR/include/QtSql \
    $$QTDIR/include/Qt \
    ../src/common \
    ../src/util \
    ../src/gui \
    ../src/sarf \
    ../src/case \
    ../src/log \
    ../src/cache \
    ../src/sql \
    ../src/build
INCLUDEPATH += $$TOP/datrie/
INCLUDEPATH += $$TOP/datrie/libdatrie-0.2.4
INCLUDEPATH += $$TOP/datrie/libdatrie-0.2.4/datrie
HEADERS += util/text_handling.h \
    ../src/util/diacritics.h \
    ../src/util/dbitvec.h \
    ../src/sql/sql_queries.h \
    ../src/sql/Search_Compatibility.h \
    ../src/sql/Search_by_item.h \
    ../src/sql/Search_by_category.h \
    ../src/sarf/tree_search.h \
    ../src/sarf/suffix_search.h \
    ../src/sarf/stemmer.h \
    ../src/sarf/stem_search.h \
    ../src/sarf/prefix_search.h \
    ../src/common/tree.h \
    ../src/common/result_node.h \
    ../src/common/node.h \
    ../src/common/letter_node.h \
    ../src/cache/database_info_block.h \
    ../src/cache/compatibility_rules.h \
    ../src/build/functions.h \
    ../src/gui/test.h \
    ../src/gui/mainwindow.h \
    ../src/log/logger.h \
    ../src/common/common.h \
    ../src/sql/Retrieve_Template.h \
    ../src/common/StemNode.h \
    ../src/common/atmTrie.h \
    ../src/case/hadith.h \
    ../src/util/letters.h \
    ../src/case/narrator_abstraction.h \
    ../src/log/ATMProgressIFC.h \
    ../src/util/Math_functions.h \
    ../src/cache/Search_by_item_locally.h \
    ../src/util/Triplet.h \
    ../src/util/Ptr.h \
    ../src/common/node_info.h \
    ../src/common/solution_position.h \
    ../src/case/chain_graph.h \
    ../src/util/reference.h \
    ../src/case/time.h
HEADERS += trie.h \
    utilities.h
SOURCES += ../src/util/text_handling.cpp \
    ../src/util/diacritics.cpp \
    ../src/util/dbitvec.cpp \
    ../src/sql/sql_queries.cpp \
    ../src/sql/Search_Compatibility.cpp \
    ../src/sql/Search_by_item.cpp \
    ../src/sql/Search_by_category.cpp \
    ../src/sarf/suffix_search.cpp \
    ../src/sarf/stemmer.cpp \
    ../src/sarf/stem_search.cpp \
    ../src/sarf/prefix_search.cpp \
    ../src/common/tree.cpp \
    ../src/common/result_node.cpp \
    ../src/common/node.cpp \
    ../src/common/letter_node.cpp \
    ../src/cache/database_info_block.cpp \
    ../src/cache/compatibility_rules.cpp \
    ../src/build/functions.cpp \
    ../src/gui/test.cpp \
    ../src/gui/mainwindow.cpp \
    ../src/sarf/tree_search.cpp \
    ../src/sql/Retrieve_Template.cpp \
    ../src/common/global_variables.cpp \
    ../src/common/StemNode.cpp \
    ../src/common/atmTrie.cpp \
    ../src/case/hadith_general.cpp \
    ../src/case/hadith_1.cpp \
    ../src/case/narrator_abstraction.cpp \
    ../src/util/Math_functions.cpp \
    ../src/cache/Search_by_item_locally.cpp \
    ../src/common/common.cpp \
    ../src/common/solution_position.cpp \
    ../src/case/chain_graph.cpp \
    ../src/case/time.cpp

# LIBS += -ldatrie
LIBS += -L$$TOP/datrie/libdatrie-0.2.4/datrie/.libs/ \
    -ldatrie
CONFIG += qt
CONFIG += debug_and_release

# -Q suppresses the call graph data
# QMAKE_CFLAGS += -pg
# QMAKE_CXXFLAGS += -pg
# QMAKE_LFLAGS += -pg
# QMAKE_CFLAGS += -O3
# QMAKE_CXXFLAGS += -O3
# QMAKE_LFLAGS += -O3
# debug
QT += sql
FORMS += ../src/gui/mainwindow.ui
OTHER_FILES += 
