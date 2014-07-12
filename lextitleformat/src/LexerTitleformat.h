#ifndef LEXTITLEFORMAT_H
#define LEXTITLEFORMAT_H

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

class LexerTitleformat : public LexerBase {
public:
	static ILexer *LexerFactoryTitleformat();

	LexerTitleformat();
	virtual ~LexerTitleformat();

	const char * SCI_METHOD PropertyNames();
	int SCI_METHOD PropertyType(const char *name);
	const char * SCI_METHOD DescribeProperty(const char *name);
	int SCI_METHOD PropertySet(const char *key, const char *val);
	const char * SCI_METHOD DescribeWordListSets();
	int SCI_METHOD WordListSet(int n, const char *wl);
	void SCI_METHOD Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
	void * SCI_METHOD PrivateCall(int operation, void *pointer);

private:
	OptionsTitleformat options;
	OptionSetTitleformat optionSet;
	LexerTitleformatPrivateCall * privateCall;
};

#endif // LEXTITLEFORMAT_H
