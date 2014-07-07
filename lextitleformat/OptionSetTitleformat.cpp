#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "Scintilla.h"

#include "OptionSet.h"

#include "OptionSetTiteformat.h"

const char * OptionSetTitleformat::wordLists[] = {
	"Control flow functions",
	"Variable access functions",
	"Built-in functions",
	"Third-party functions",
	0,
};


OptionSetTitleformat::OptionSetTitleformat() {
	DefineProperty("fold", &OptionsTitleformat::fold, "");

	DefineWordListSets(wordLists);
}
