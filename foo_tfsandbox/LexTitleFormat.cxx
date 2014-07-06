// Scintilla source code edit control
/** @file LexTitleFormat.cxx
 ** Lexer for foobar2000 title formatting language.
 **
 ** Written by Holger Stenger.
 **/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

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

static void ColouriseTitleFormatDoc(
	unsigned int startPos,
	int length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler) {

#if 0
	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];
	WordList &keywords6 = *keywordlists[5];
	WordList &keywords7 = *keywordlists[6];
	WordList &keywords8 = *keywordlists[7];
#else
	// Avoid unused parameter warning
	keywordlists;
#endif

	// Do not leak onto next line
	if (initStyle == SCE_TITLEFORMAT_COMMENTLINE) {
		initStyle = SCE_TITLEFORMAT_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);
	for (; sc.More(); sc.Forward()) {
		// Determine if the current state should terminate.
		if (sc.state == SCE_TITLEFORMAT_OPERATOR) {
			sc.SetState(SCE_TITLEFORMAT_DEFAULT);
		} else if (sc.state == SCE_TITLEFORMAT_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
#if 0
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_TITLEFORMAT_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_TITLEFORMAT_WORD2);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_TITLEFORMAT_WORD3);
				} else if (keywords4.InList(s)) {
					sc.ChangeState(SCE_TITLEFORMAT_WORD4);
				} else if (keywords5.InList(s)) {
					sc.ChangeState(SCE_TITLEFORMAT_WORD5);
				} else if (keywords6.InList(s)) {
					sc.ChangeState(SCE_TITLEFORMAT_WORD6);
				} else if (keywords7.InList(s)) {
					sc.ChangeState(SCE_TITLEFORMAT_WORD7);
				} else if (keywords8.InList(s)) {
					sc.ChangeState(SCE_TITLEFORMAT_WORD8);
				}
#endif
				sc.SetState(SCE_TITLEFORMAT_DEFAULT);
			}
		} else if (sc.state == SCE_TITLEFORMAT_COMMENTLINE) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_TITLEFORMAT_DEFAULT);
			}
		} else if (sc.state == SCE_TITLEFORMAT_STRING) {
			if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_TITLEFORMAT_DEFAULT);
			}
		} else if (sc.state == SCE_TITLEFORMAT_LITERALSTRING) {
			if (!IsALiteralStringChar(sc.ch)) {
				sc.SetState(SCE_TITLEFORMAT_DEFAULT);
			}
		} else if (sc.state == SCE_TITLEFORMAT_SPECIALSTRING) {
			sc.SetState(SCE_TITLEFORMAT_DEFAULT);
		} else if (sc.state == SCE_TITLEFORMAT_FIELD) {
			if (sc.ch == '%') {
				sc.ForwardSetState(SCE_TITLEFORMAT_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_TITLEFORMAT_DEFAULT) {
			if (sc.Match('/', '/')) {
				sc.SetState(SCE_TITLEFORMAT_COMMENTLINE);
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

static void FoldTitleFormatDoc(unsigned int startPos, int length, int /* initStyle */, WordList *[],
                       Accessor &styler) {
#if 0
	unsigned int lengthDoc = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	int styleNext = styler.StyleAt(startPos);
	char s[10];

	for (unsigned int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (style == SCE_TITLEFORMAT_WORD) {
			if (ch == 'i' || ch == 'd' || ch == 'f' || ch == 'e' || ch == 'r' || ch == 'u') {
				for (unsigned int j = 0; j < 8; j++) {
					if (!iswordchar(styler[i + j])) {
						break;
					}
					s[j] = styler[i + j];
					s[j + 1] = '\0';
				}

				if ((strcmp(s, "if") == 0) || (strcmp(s, "do") == 0) || (strcmp(s, "function") == 0) || (strcmp(s, "repeat") == 0)) {
					levelCurrent++;
				}
				if ((strcmp(s, "end") == 0) || (strcmp(s, "elseif") == 0) || (strcmp(s, "until") == 0)) {
					levelCurrent--;
				}
			}
		} else if (style == SCE_TITLEFORMAT_OPERATOR) {
			if (ch == '{' || ch == '(') {
				levelCurrent++;
			} else if (ch == '}' || ch == ')') {
				levelCurrent--;
			}
		} else if (style == SCE_TITLEFORMAT_LITERALSTRING || style == SCE_TITLEFORMAT_COMMENT) {
			if (ch == '[') {
				levelCurrent++;
			} else if (ch == ']') {
				levelCurrent--;
			}
		}

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact) {
				lev |= SC_FOLDLEVELWHITEFLAG;
			}
			if ((levelCurrent > levelPrev) && (visibleChars > 0)) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later

	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
#else
	unsigned int lengthDoc = startPos + length;
	int lineCurrent = styler.GetLine(startPos);
	char chNext = styler[startPos];

	for (unsigned int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (atEOL) {
			lineCurrent++;
			styler.SetLevel(lineCurrent, 0);
		}
	}
#endif
}

static const char * const titleFormatWordListDesc[] = {
	"Control flow functions",
	"Basic functions",
	"String, and math functions",
	"user1",
	"user2",
	"user3",
	"user4",
	"user5",
	0
};

LexerModule lmTitleFormat(SCLEX_TITLEFORMAT, ColouriseTitleFormatDoc, "titleformat", FoldTitleFormatDoc, titleFormatWordListDesc);
