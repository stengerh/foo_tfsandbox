#ifndef OPTIONSETTITLEFORMAT_H
#define OPTIONSETTITLEFORMAT_H

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

struct OptionsTitleformat {
	bool fold;
	OptionsTitleformat();
};

class OptionSetTitleformat : public OptionSet<OptionsTitleformat> {
public:
	static const char * wordLists[];

	OptionSetTitleformat();
};

#endif // OPTIONSETTITLEFORMAT_H
