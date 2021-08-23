#-------------------------------------------------------------------------------
# Sample makefile for building the code samples. 
# Edited by Qian Zhang Dec 27, 2020
# Rewrote by Jiyuan Wang Jan 10,
#-------------------------------------------------------------------------------

LLVM_SRC_PATH := /usr/local/opt/llvm/

LLVM_BUILD_PATH := /usr/local/opt/llvm
LLVM_BIN_PATH 	:= $(LLVM_BUILD_PATH)/bin

$(info -----------------------------------------------)
$(info Using LLVM_SRC_PATH = $(LLVM_SRC_PATH))
$(info Using LLVM_BUILD_PATH = $(LLVM_BUILD_PATH))
$(info Using LLVM_BIN_PATH = $(LLVM_BIN_PATH))
$(info -----------------------------------------------)

CC := /usr/local/opt/llvm/bin/clang
CXX := $(CC)++
CXXFLAGS := -fno-rtti -O0 -g
PLUGIN_CXXFLAGS := -fpic

LLVM_CXXFLAGS := `$(LLVM_BIN_PATH)/llvm-config --cxxflags`
LLVM_LDFLAGS := `$(LLVM_BIN_PATH)/llvm-config --ldflags --libs --system-libs`

LLVM_LDFLAGS_NOLIBS := `$(LLVM_BIN_PATH)/llvm-config --ldflags`
PLUGIN_LDFLAGS := -shared

# These are required when compiling vs. a source distribution of Clang. For
# binary distributions llvm-config --cxxflags gives the right path.
CLANG_INCLUDES := \
	-I$(LLVM_SRC_PATH)/tools/clang/include \
	-I$(LLVM_BUILD_PATH)/tools/clang/include

CLANG_LIBS := \
	-lclangAST \
	-lclangASTMatchers \
	-lclangAnalysis \
	-lclangBasic \
	-lclangDriver \
	-lclangEdit \
	-lclangFrontend \
	-lclangFrontendTool \
	-lclangLex \
	-lclangParse \
	-lclangSema \
	-lclangEdit \
	-lclangRewrite \
	-lclangRewriteFrontend \
	-lclangStaticAnalyzerFrontend \
	-lclangStaticAnalyzerCheckers \
	-lclangStaticAnalyzerCore \
	-lclangCrossTU \
	-lclangIndex \
	-lclangSerialization \
	-lclangToolingCore \
	-lclangTooling \
	-lclangFormat 

# Internal paths in this project
SRC_CLANG_DIR := src
BUILDDIR := build

.PHONY: all
all: make_builddir \
	emit_build_config \
		$(BUILDDIR)/pattern6 \
		$(BUILDDIR)/pattern5 \
		$(BUILDDIR)/pattern1 \
		$(BUILDDIR)/pattern4 \
		$(BUILDDIR)/pattern2 \
		$(BUILDDIR)/pattern_extract_14 \
		$(BUILDDIR)/pattern_extract_5 \
		$(BUILDDIR)/pattern_extract_236 

.PHONY: test
test: emit_build_config
	python3 test/all_tests.py

.PHONY: emit_build_config
emit_build_config: make_builddir
	@echo $(LLVM_BIN_PATH) > $(BUILDDIR)/_build_config

.PHONY: make_builddir
make_builddir:
	@test -d $(BUILDDIR) || mkdir $(BUILDDIR)


$(BUILDDIR)/pattern6: $(SRC_CLANG_DIR)/pattern6.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/pattern5: $(SRC_CLANG_DIR)/pattern5.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/pattern1: $(SRC_CLANG_DIR)/pattern1.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/pattern4: $(SRC_CLANG_DIR)/pattern4.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/pattern2: $(SRC_CLANG_DIR)/pattern2.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/pattern_extract_14: $(SRC_CLANG_DIR)/pattern_extract_14.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/pattern_extract_5: $(SRC_CLANG_DIR)/pattern_extract_5.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/pattern_extract_236: $(SRC_CLANG_DIR)/pattern_extract_236.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@


# Experimental tools
.PHONY: experimental_tools
experimental_tools: make_builddir \
	emit_build_config \
	$(BUILDDIR)/loop_info \
	$(BUILDDIR)/build_llvm_ir \
	$(BUILDDIR)/remove-cstr-calls \
	$(BUILDDIR)/toplevel_decls \
	$(BUILDDIR)/try_matcher

$(BUILDDIR)/loop_info: $(SRC_LLVM_DIR)/experimental/loop_info.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $^ $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/build_llvm_ir: $(SRC_LLVM_DIR)/experimental/build_llvm_ir.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $^ -rdynamic $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/remove-cstr-calls: $(SRC_CLANG_DIR)/experimental/RemoveCStrCalls.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/toplevel_decls: $(SRC_CLANG_DIR)/experimental/toplevel_decls.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

$(BUILDDIR)/try_matcher: $(SRC_CLANG_DIR)/experimental/try_matcher.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

.PHONY: clean format

clean:
	rm -rf $(BUILDDIR)/* *.dot test/*.pyc test/__pycache__

format:
	find . -name "*.cpp" | xargs clang-format -style=file -i
