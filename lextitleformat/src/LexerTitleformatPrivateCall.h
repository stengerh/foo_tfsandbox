#ifndef LEXERTITLEFORMATPRIVATECALL_H
#define LEXERTITLEFORMATPRIVATECALL_H

typedef int Position;

class Range {
public:
	Position start;
	Position end;

	Range(Position pos = 0) :
		start(pos), end(pos) {
	}

	Range(Position start_, Position end_) :
		start(start_), end(end_) {
	}

	// Is the character after pos within the range?
	bool ContainsCharacter(Position pos) const {
		return (pos >= start && pos < end);
	}
};

class LexerTitleformatPrivateCall : public ILexerTitleformatPrivateCall {
public:
	LexerTitleformatPrivateCall();
	virtual ~LexerTitleformatPrivateCall();

	int SCI_METHOD Version() const {
		return ltfvOriginal;
	}

	void AddRef();
	void SCI_METHOD Release();

	void SCI_METHOD ClearInactiveRanges();
	void SCI_METHOD SetInactiveRanges(int count, Sci_CharacterRange * ranges);
	int GetInactiveRangeCount() const;
	bool InactiveRangeContainsCharacter(int index, Position pos) const;
	int GetInactiveRangeLowerBound(Position pos) const;
private:
	int refCount;
	std::vector<Range> inactiveRanges;
};

#endif
