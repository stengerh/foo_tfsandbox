#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "LexerModule.h"
#include "LexerBase.h"
#include "OptionSet.h"
#include "StyleContext.h"

#include "OptionSetTiteformat.h"
#include "LexerTitleformat.h"

static inline bool IsAWordChar(int ch) {
	return isalnum(ch) || ch == '_';
}

static inline bool IsALiteralStringChar(int ch) {
	return ch != '$' && ch != '%' && ch != '\'' &&
		   ch != '(' && ch != ')' && ch != ',' &&
		   ch != '[' && ch != ']' &&
		   ch != '\r' && ch != '\n';
}

static inline bool IsASpecialStringChar(int ch) {
	return ch == '$' || ch == '%' || ch == '\'';
}

static inline bool IsTitleFormatOperator(int ch) {
	if (ch == '(' || ch == ')' || ch == ',' ||
		ch == '[' || ch == ']') {
		return true;
	}
	return false;
}


ILexer *LexerTitleformat::LexerFactoryTitleformat() {
	return new LexerTitleformat();
}

LexerTitleformat::LexerTitleformat() {
}

LexerTitleformat::~LexerTitleformat() {
}

const char * LexerTitleformat::PropertyNames() {
	return optionSet.PropertyNames();
}

int LexerTitleformat::PropertyType(const char *name) {
	return optionSet.PropertyType(name);
}

const char * LexerTitleformat::DescribeProperty(const char *name) {
	return optionSet.DescribeProperty(name);
}

int LexerTitleformat::PropertySet(const char *key, const char *val) {
	return optionSet.PropertySet(&options, key, val);
}

const char * SCI_METHOD LexerTitleformat::DescribeWordListSets() {
	return optionSet.DescribeWordListSets();
}

int LexerTitleformat::WordListSet(int n, const char *wl) {
	return LexerBase::WordListSet(n, wl);
}

void LexerTitleformat::Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);
	StyleContext sc = StyleContext(startPos, lengthDoc, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		// Determine if the current state should terminate.
		switch (sc.state) {
		case SCE_TITLEFORMAT_OPERATOR:
			sc.SetState(SCE_TITLEFORMAT_DEFAULT);
			break;
		case SCE_TITLEFORMAT_IDENTIFIER:
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_TITLEFORMAT_DEFAULT);
			}
			break;
		case SCE_TITLEFORMAT_COMMENT:
			if (sc.atLineStart) {
				sc.SetState(SCE_TITLEFORMAT_DEFAULT);
			}
			break;
		case SCE_TITLEFORMAT_STRING:
			if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_TITLEFORMAT_DEFAULT);
			}
			break;
		case SCE_TITLEFORMAT_LITERALSTRING:
			if (!IsALiteralStringChar(sc.ch)) {
				sc.SetState(SCE_TITLEFORMAT_DEFAULT);
			}
			break;
		case SCE_TITLEFORMAT_SPECIALSTRING:
			sc.SetState(SCE_TITLEFORMAT_DEFAULT);
			break;
		case SCE_TITLEFORMAT_FIELD:
			if (sc.ch == '%') {
				sc.ForwardSetState(SCE_TITLEFORMAT_DEFAULT);
			}
			break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_TITLEFORMAT_DEFAULT) {
			if (sc.Match('/', '/')) {
				sc.SetState(SCE_TITLEFORMAT_COMMENT);
				sc.Forward();
			} else if (IsTitleFormatOperator(sc.ch)) {
				sc.SetState(SCE_TITLEFORMAT_OPERATOR);
			} else if (IsASpecialStringChar(sc.ch) && sc.ch == sc.chNext) {
				sc.SetState(SCE_TITLEFORMAT_SPECIALSTRING);
				sc.Forward();
			} else if (sc.ch == '$') {
				sc.SetState(SCE_TITLEFORMAT_IDENTIFIER);
			} else if (sc.ch == '%') {
				sc.SetState(SCE_TITLEFORMAT_FIELD);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_TITLEFORMAT_STRING);
			} else if (IsALiteralStringChar(sc.ch)) {
				sc.SetState(SCE_TITLEFORMAT_LITERALSTRING);
			}
		}
	}
	sc.Complete();
}

void LexerTitleformat::Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess) {
	if (!options.fold) {
		return;
	}

	LexAccessor styler(pAccess);

	int startLine = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(startLine) & SC_FOLDLEVELNUMBERMASK;

	int lastLine = styler.GetLine(lengthDoc);

	for (int line = startLine; line < lastLine; ++line) {
		int level = 0;

		if (styler.Match(styler.LineStart(line), "//")) {
			level = 1;
		}

		int levelAndFlags = level;

		if (level > levelPrev) {
			levelAndFlags |= SC_FOLDLEVELHEADERFLAG;
		}

		styler.SetLevel(line, levelAndFlags);

		levelPrev = level;
	}
}

void * LexerTitleformat::PrivateCall(int operation, void *pointer) {
	return 0;
}
