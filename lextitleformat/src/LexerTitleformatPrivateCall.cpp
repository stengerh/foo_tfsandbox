#include <vector>
#include <algorithm>

#include <Scintilla.h>
#include <ILexer.h>

#include <ILexerTitleformat.h>

#include "LexerTitleformatPrivateCall.h"

LexerTitleformatPrivateCall::LexerTitleformatPrivateCall() {
	refCount = 1;
}

LexerTitleformatPrivateCall::~LexerTitleformatPrivateCall() {
}

void LexerTitleformatPrivateCall::AddRef() {
	++refCount;
}

void LexerTitleformatPrivateCall::Release() {
	int curRefCount = --refCount;
	if (curRefCount == 0) {
		delete this;
	}
}

void LexerTitleformatPrivateCall::ClearInactiveRanges() {
	inactiveRanges.clear();
}

void LexerTitleformatPrivateCall::SetInactiveRanges(int count, Sci_CharacterRange * ranges) {
	inactiveRanges.resize(count);
	for (int index = 0; index < count; ++index) {
		inactiveRanges[index] = Range(ranges[index].cpMin, ranges[index].cpMax);
	}
}

int LexerTitleformatPrivateCall::GetInactiveRangeCount() const {
	return inactiveRanges.size();
}

bool LexerTitleformatPrivateCall::InactiveRangeContainsCharacter(int index, Position pos) const {
	if (index < 0 || index >= inactiveRanges.size()) {
		return false;
	} else {
		return inactiveRanges[index].ContainsCharacter(pos);
	}
}

int LexerTitleformatPrivateCall::GetInactiveRangeLowerBound(Position pos) const {
	struct pred {
		bool operator ()(const Range & range, Position pos) {
			return range.end < pos;
		}
	};

	auto it = std::lower_bound(inactiveRanges.begin(), inactiveRanges.end(), pos, pred());

	return it - inactiveRanges.begin();
}
