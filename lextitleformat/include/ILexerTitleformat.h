#ifndef ILEXTITLEFORMAT_H
#define ILEXTITLEFORMAT_H

#ifndef SCI_METHOD
#ifdef _WIN32
	#define SCI_METHOD __stdcall
#else
	#define SCI_METHOD
#endif
#endif

enum SCE_TITLEFORMAT {
	SCE_TITLEFORMAT_DEFAULT = 0,
	SCE_TITLEFORMAT_COMMENT = 1,
	SCE_TITLEFORMAT_OPERATOR = 2,
	SCE_TITLEFORMAT_FIELD = 3,
	SCE_TITLEFORMAT_STRING = 4,
	SCE_TITLEFORMAT_LITERALSTRING = 5,
	SCE_TITLEFORMAT_SPECIALSTRING = 6,
	SCE_TITLEFORMAT_IDENTIFIER = 7,
};

enum {
	SPC_TITLEFORMAT_GETINTERFACE = 1,
};

enum { ltfvOriginal = 0 };

class ILexerTitleformatPrivateCall {
public:
	virtual int SCI_METHOD Version() const = 0;
	virtual void SCI_METHOD Release() = 0;
	virtual void SCI_METHOD ClearInactiveRanges() = 0;
	virtual void SCI_METHOD SetInactiveRanges(int count, Sci_CharacterRange * ranges) = 0;
};

#endif // ILEXTITLEFORMAT_H
