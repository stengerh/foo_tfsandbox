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
#include "LexerModule.h"
#include "OptionSet.h"

#include "ILexerTitleformat.h"

#include "LexerTitleformatPrivateCall.h"
#include "OptionSetTiteformat.h"
#include "LexerTitleformat.h"

#ifdef BUILD_AS_EXTERNAL_LEXER

#ifdef _WIN32
#define EXT_LEXER_DECL __declspec( dllexport ) __stdcall
#else
#define EXT_LEXER_DECL
#endif

static const char *lexerName = "titleformat";

extern "C" {

int EXT_LEXER_DECL GetLexerCount() {
        return 1;
}

void EXT_LEXER_DECL GetLexerName(unsigned int index, char *name, int buflength) {
        *name = 0;
        if ((index == 0) && (buflength > static_cast<int>(strlen(lexerName)))) {
                strcpy(name, lexerName);
        }
}

LexerFactoryFunction EXT_LEXER_DECL GetLexerFactory(unsigned int index) {
        if (index == 0)
                return LexerTitleformat::LexerFactoryTitleformat;
        else
                return 0;
}

}

#else

LexerModule lmTitleformat(SCLEX_AUTOMATIC, LexerTitleformat::LexerFactoryTitleformat, "titleformat", OptionSetTitleformat::wordLists);

#endif
