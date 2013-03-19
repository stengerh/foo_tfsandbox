#include "stdafx.h"

#include <memory.h>
#include <string.h>
#include "CustomScanner.h"

// string handling, ascii/utf-8 character

char* coco_string_create(const char* value) {
	char* data;
	int len = 0;
	if (value) { len = strlen(value); }
	data = new char[len + 1];
	strncpy(data, value, len);
	data[len] = 0;
	return data;
}

char* coco_string_create(const char *value , int startIndex, int length) {
	int len = 0;
	char* data;

	if (value) { len = length; }
	data = new char[len + 1];
	strncpy(data, &(value[startIndex]), len);
	data[len] = 0;

	return data;
}

char* coco_string_create_upper(const char* data) {
	if (!data) { return NULL; }

	int dataLen = 0;
	if (data) { dataLen = strlen(data); }

	char *newData = new char[dataLen + 1];

	for (int i = 0; i <= dataLen; i++) {
		if ((L'a' <= data[i]) && (data[i] <= L'z')) {
			newData[i] = data[i] + (L'A' - L'a');
		}
		else { newData[i] = data[i]; }
	}

	newData[dataLen] = L'\0';
	return newData;
}

char* coco_string_create_lower(const char* data) {
	if (!data) { return NULL; }
	int dataLen = strlen(data);
	return coco_string_create_lower(data, 0, dataLen);
}

char* coco_string_create_lower(const char* data, int startIndex, int dataLen) {
	if (!data) { return NULL; }

	char* newData = new char[dataLen + 1];

	for (int i = 0; i <= dataLen; i++) {
		char ch = data[startIndex + i];
		if ((L'A' <= ch) && (ch <= L'Z')) {
			newData[i] = ch - (L'A' - L'a');
		}
		else { newData[i] = ch; }
	}
	newData[dataLen] = L'\0';
	return newData;
}

char* coco_string_create_append(const char* data1, const char* data2) {
	char* data;
	int data1Len = 0;
	int data2Len = 0;

	if (data1) { data1Len = strlen(data1); }
	if (data2) {data2Len = strlen(data2); }

	data = new char[data1Len + data2Len + 1];

	if (data1) { strcpy(data, data1); }
	if (data2) { strcpy(data + data1Len, data2); }

	data[data1Len + data2Len] = 0;

	return data;
}

char* coco_string_create_append(const char *target, const char appendix) {
	int targetLen = coco_string_length(target);
	char* data = new char[targetLen + 2];
	strncpy(data, target, targetLen);
	data[targetLen] = appendix;
	data[targetLen + 1] = 0;
	return data;
}

void coco_string_delete(char* &data) {
	delete [] data;
	data = NULL;
}

int coco_string_length(const char* data) {
	if (data) { return strlen(data); }
	return 0;
}

bool coco_string_endswith(const char* data, const char *end) {
	int dataLen = strlen(data);
	int endLen = strlen(end);
	return (endLen <= dataLen) && (strcmp(data + dataLen - endLen, end) == 0);
}

int coco_string_indexof(const char* data, const char value) {
	const char* chr = strchr(data, value);

	if (chr) { return (chr-data); }
	return -1;
}

int coco_string_lastindexof(const char* data, const char value) {
	const char* chr = strrchr(data, value);

	if (chr) { return (chr-data); }
	return -1;
}

void coco_string_merge(char* &target, const char* appendix) {
	if (!appendix) { return; }
	char* data = coco_string_create_append(target, appendix);
	delete [] target;
	target = data;
}

bool coco_string_equal(const char* data1, const char* data2) {
	return strcmp( data1, data2 ) == 0;
}

int coco_string_compareto(const char* data1, const char* data2) {
	return strcmp(data1, data2);
}

int coco_string_hash(const char *data) {
	int h = 0;
	if (!data) { return 0; }
	while (*data != 0) {
		h = (h * 7) ^ *data;
		++data;
	}
	if (h < 0) { h = -h; }
	return h;
}

// string handling, wide character

wchar_t* coco_string_create(const wchar_t* value) {
	wchar_t* data;
	int len = 0;
	if (value) { len = wcslen(value); }
	data = new wchar_t[len + 1];
	wcsncpy(data, value, len);
	data[len] = 0;
	return data;
}

void coco_string_delete(wchar_t* &data) {
	delete [] data;
	data = NULL;
}


Token::Token() {
	kind = 0;
	pos  = 0;
	col  = 0;
	line = 0;
	val  = NULL;
	next = NULL;
}

Token::~Token() {
	coco_string_delete(val);
}

Buffer::Buffer(Buffer *b) {
	buf = b->buf;
	bufCapacity = b->bufCapacity;
	b->buf = NULL;
	bufStart = b->bufStart;
	bufLen = b->bufLen;
	fileLen = b->fileLen;
	bufPos = b->bufPos;
	stream = b->stream;
	b->stream = NULL;
	isUserStream = b->isUserStream;
}

Buffer::Buffer(const unsigned char* buf, int len) {
	this->buf = new unsigned char[len];
	memcpy(this->buf, buf, len*sizeof(unsigned char));
	bufStart = 0;
	bufCapacity = bufLen = len;
	fileLen = len;
	bufPos = 0;
	stream = NULL;
}

Buffer::~Buffer() {
	Close(); 
	if (buf != NULL) {
		delete [] buf;
		buf = NULL;
	}
}

void Buffer::Close() {
	if (!isUserStream && stream != NULL) {
		fclose(stream);
		stream = NULL;
	}
}

int Buffer::Read() {
	if (bufPos < bufLen) {
		return buf[bufPos++];
	} else if (GetPos() < fileLen) {
		SetPos(GetPos()); // shift buffer start to Pos
		return buf[bufPos++];
	} else if ((stream != NULL) && !CanSeek() && (ReadNextStreamChunk() > 0)) {
		return buf[bufPos++];
	} else {
		return EoF;
	}
}

int Buffer::Peek() {
	int curPos = GetPos();
	int ch = Read();
	SetPos(curPos);
	return ch;
}

char* Buffer::GetString(int beg, int end) {
	int len = end - beg;
	char *buf = new char[len];
	int oldPos = GetPos();
	SetPos(beg);
	for (int i = 0; i < len; ++i) buf[i] = (char) Read();
	SetPos(oldPos);
	return buf;
}

int Buffer::GetPos() {
	return bufPos + bufStart;
}

void Buffer::SetPos(int value) {
	if ((value >= fileLen) && (stream != NULL) && !CanSeek()) {
		// Wanted position is after buffer and the stream
		// is not seek-able e.g. network or console,
		// thus we have to read the stream manually till
		// the wanted position is in sight.
		while ((value >= fileLen) && (ReadNextStreamChunk() > 0));
	}

	if ((value < 0) || (value > fileLen)) {
		pfc::string_formatter msg;
		msg << "buffer out of bounds access, position: " << value;
		pfc::dynamic_assert(false, msg);
	}

	if ((value >= bufStart) && (value < (bufStart + bufLen))) { // already in buffer
		bufPos = value - bufStart;
	} else if (stream != NULL) { // must be swapped in
		fseek(stream, value, SEEK_SET);
		bufLen = fread(buf, sizeof(unsigned char), bufCapacity, stream);
		bufStart = value; bufPos = 0;
	} else {
		bufPos = fileLen - bufStart; // make Pos return fileLen
	}
}

// Read the next chunk of bytes from the stream, increases the buffer
// if needed and updates the fields fileLen and bufLen.
// Returns the number of bytes read.
int Buffer::ReadNextStreamChunk() {
	int free = bufCapacity - bufLen;
	if (free == 0) {
		// in the case of a growing input stream
		// we can neither seek in the stream, nor can we
		// foresee the maximum length, thus we must adapt
		// the buffer size on demand.
		bufCapacity = bufLen * 2;
		unsigned char *newBuf = new unsigned char[bufCapacity];
		memcpy(newBuf, buf, bufLen*sizeof(unsigned char));
		delete [] buf;
		buf = newBuf;
		free = bufLen;
	}
	int read = fread(buf + bufLen, sizeof(unsigned char), free, stream);
	if (read > 0) {
		fileLen = bufLen = (bufLen + read);
		return read;
	}
	// end of stream reached
	return 0;
}

bool Buffer::CanSeek() {
	return (stream != NULL) && (ftell(stream) != -1);
}

int UTF8Buffer::Read() {
	int ch;
	do {
		ch = Buffer::Read();
		// until we find a uft8 start (0xxxxxxx or 11xxxxxx)
	} while ((ch >= 128) && ((ch & 0xC0) != 0xC0) && (ch != EoF));
	if (ch < 128 || ch == EoF) {
		// nothing to do, first 127 chars are the same in ascii and utf8
		// 0xxxxxxx or end of file character
	} else if ((ch & 0xF0) == 0xF0) {
		// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		int c1 = ch & 0x07; ch = Buffer::Read();
		int c2 = ch & 0x3F; ch = Buffer::Read();
		int c3 = ch & 0x3F; ch = Buffer::Read();
		int c4 = ch & 0x3F;
		ch = (((((c1 << 6) | c2) << 6) | c3) << 6) | c4;
	} else if ((ch & 0xE0) == 0xE0) {
		// 1110xxxx 10xxxxxx 10xxxxxx
		int c1 = ch & 0x0F; ch = Buffer::Read();
		int c2 = ch & 0x3F; ch = Buffer::Read();
		int c3 = ch & 0x3F;
		ch = (((c1 << 6) | c2) << 6) | c3;
	} else if ((ch & 0xC0) == 0xC0) {
		// 110xxxxx 10xxxxxx
		int c1 = ch & 0x1F; ch = Buffer::Read();
		int c2 = ch & 0x3F;
		ch = (c1 << 6) | c2;
	}
	return ch;
}

Scanner::Scanner(const unsigned char* buf, int len) {
	buffer = new Buffer(buf, len);
	Init();
}

Scanner::~Scanner() {
	char* cur = (char*) firstHeap;

	while(cur != NULL) {
		cur = *(char**) (cur + HEAP_BLOCK_SIZE);
		free(firstHeap);
		firstHeap = cur;
	}
	delete [] tval;
	delete buffer;
}

void Scanner::Init() {
	EOL    = '\n';
	eofSym = 0;
	maxT = 11;
	noSym = 11;

#if 0
	int i;
	for (i = 0; i <= 9; ++i) start.set(i, 1);
	for (i = 11; i <= 12; ++i) start.set(i, 1);
	for (i = 14; i <= 35; ++i) start.set(i, 1);
	for (i = 38; i <= 38; ++i) start.set(i, 1);
	for (i = 42; i <= 43; ++i) start.set(i, 1);
	for (i = 45; i <= 90; ++i) start.set(i, 1);
	for (i = 92; i <= 92; ++i) start.set(i, 1);
	for (i = 94; i <= 65535; ++i) start.set(i, 1);
	start.set(39, 13);
	start.set(37, 14);
	start.set(36, 15);
	start.set(91, 8);
	start.set(93, 9);
	start.set(40, 10);
	start.set(41, 11);
	start.set(44, 12);
		start.set(Buffer::EoF, -1);
#endif


	tvalLength = 128;
	tval = new char[tvalLength]; // text of current token

	// HEAP_BLOCK_SIZE byte heap + pointer to next heap block
	heap = malloc(HEAP_BLOCK_SIZE + sizeof(void*));
	firstHeap = heap;
	heapEnd = (void**) (((char*) heap) + HEAP_BLOCK_SIZE);
	*heapEnd = 0;
	heapTop = heap;
	if (sizeof(Token) > HEAP_BLOCK_SIZE) {
		pfc::dynamic_assert(false, "Too small HEAP_BLOCK_SIZE");
	}

	pos = -1; line = 1; col = 0;
	oldEols = 0;
	NextCh();
	if (ch == 0xEF) { // check optional byte order mark for UTF-8
		NextCh(); int ch1 = ch;
		NextCh(); int ch2 = ch;
		if (ch1 != 0xBB || ch2 != 0xBF) {
			pfc::dynamic_assert(false, "Illegal byte order mark at start of file");
		}
		Buffer *oldBuf = buffer;
		buffer = new UTF8Buffer(buffer); col = 0;
		delete oldBuf; oldBuf = NULL;
		NextCh();
	}


	pt = tokens = CreateToken(); // first token is a dummy
}

void Scanner::NextCh() {
	if (oldEols > 0) { ch = EOL; oldEols--; }
	else {
		pos = buffer->GetPos();
		ch = buffer->Read();
		if (ch != Buffer::EoF) col++;
		// replace isolated '\r' by '\n' in order to make
		// eol handling uniform across Windows, Unix and Mac
		if (ch == L'\r' && buffer->Peek() != L'\n') ch = EOL;
		if (ch == EOL) { line++; col = 0; }
	}

}

void Scanner::AddCh() {
	if (tlen >= tvalLength) {
		tvalLength *= 2;
		char *newBuf = new char[tvalLength];
		memcpy(newBuf, tval, tlen*sizeof(char));
		delete [] tval;
		tval = newBuf;
	}
	tval[tlen++] = ch;
	NextCh();
}


#if 0
bool Scanner::Comment0() {
	int level = 1, pos0 = pos, line0 = line, col0 = col;
	NextCh();
	if (ch == L'/') {
		NextCh();
		for(;;) {
			if (ch == 10) {
				level--;
				if (level == 0) { oldEols = line - line0; NextCh(); return true; }
				NextCh();
			} else if (ch == buffer->EoF) return false;
			else NextCh();
		}
	} else {
		buffer->SetPos(pos0); NextCh(); line = line0; col = col0;
	}
	return false;
}
#endif

void Scanner::CreateHeapBlock() {
	void* newHeap;
	char* cur = (char*) firstHeap;

	while(((char*) tokens < cur) || ((char*) tokens > (cur + HEAP_BLOCK_SIZE))) {
		cur = *((char**) (cur + HEAP_BLOCK_SIZE));
		free(firstHeap);
		firstHeap = cur;
	}

	// HEAP_BLOCK_SIZE byte heap + pointer to next heap block
	newHeap = malloc(HEAP_BLOCK_SIZE + sizeof(void*));
	*heapEnd = newHeap;
	heapEnd = (void**) (((char*) newHeap) + HEAP_BLOCK_SIZE);
	*heapEnd = 0;
	heap = newHeap;
	heapTop = heap;
}

Token* Scanner::CreateToken() {
	Token *t;
	if (((char*) heapTop + (int) sizeof(Token)) >= (char*) heapEnd) {
		CreateHeapBlock();
	}
	t = (Token*) heapTop;
	heapTop = (void*) ((char*) heapTop + sizeof(Token));
	t->val = NULL;
	t->next = NULL;
	return t;
}

void Scanner::AppendVal(Token *t) {
	int reqMem = (tlen + 1) * sizeof(char);
	if (((char*) heapTop + reqMem) >= (char*) heapEnd) {
		if (reqMem > HEAP_BLOCK_SIZE) {
			pfc::dynamic_assert(false, "Too long token value");
		}
		CreateHeapBlock();
	}
	t->val = (char*) heapTop;
	heapTop = (void*) ((char*) heapTop + reqMem);

	strncpy(t->val, tval, tlen);
	t->val[tlen] = L'\0';
}

Token* Scanner::NextToken() {
	while (ch == 10 || ch == 13) NextCh();
	t = CreateToken();
	t->pos = pos; t->col = col; t->line = line;
	int ch0 = ch;
	tlen = 0; AddCh();

	switch (ch0)
	{
	case Buffer::EoF:
		{t->kind = eofSym; break;}
	case L'\'':
		if (ch == L'\'') {AddCh(); t->kind = 5; break;}
		else while (ch != L'\'' && ch <= 65535) {AddCh();}
		if (ch == L'\'') {AddCh(); t->kind = 2; break;}
		else {t->kind = 11; break;}
	case L'%':
		if (ch == L'%') {AddCh(); t->kind = 5; break;}
		else while (ch != L'\r' && ch != L'\n' && ch != L'%' && ch <= 65535) {AddCh();}
		if (ch == L'%') {AddCh(); t->kind = 3; break;}
		else {t->kind = 12; break;}
	case L'$':
		if (ch == L'$') {AddCh(); t->kind = 5; break;}
		else if (ch == L'\r' || ch == L'\n' || ch == Buffer::EoF) {t->kind = 13; break;}
		else while (ch != L'\r' && ch != L'\n' && ch != L'(' && ch <= 65535) {AddCh();}
		{t->kind = 4; break;}
	case L'[':
		{t->kind = 6; break;}
	case L']':
		{t->kind = 7; break;}
	case L'(':
		{t->kind = 8; break;}
	case L')':
		{t->kind = 9; break;}
	case L',':
		{t->kind = 10; break;}
		break;
	case L'/':
		if (ch == L'/') {AddCh(); while (ch != L'\r' && ch != '\n' && ch <= 65535) AddCh(); t->kind = 14; break;}
		// fall through
	default:
		while (ch != L'\r' && ch != L'\n' && ch != L'%' && ch != L'\'' && ch != L'$' && ch != L'(' && ch != L')' && ch != L',' && ch != L'[' && ch != L']' && ch <= 65535) {AddCh();}
		{t->kind = 1; break;}
	}
	AppendVal(t);
	return t;
}

// get the next token (possibly a token already seen during peeking)
Token* Scanner::Scan() {
	if (tokens->next == NULL) {
		return pt = tokens = NextToken();
	} else {
		pt = tokens = tokens->next;
		return tokens;
	}
}

// peek for the next token, ignore pragmas
Token* Scanner::Peek() {
	if (pt->next == NULL) {
		do {
			pt = pt->next = NextToken();
		} while (pt->kind > maxT); // skip pragmas
	} else {
		do {
			pt = pt->next;
		} while (pt->kind > maxT);
	}
	return pt;
}

// make sure that peeking starts at the current scan position
void Scanner::ResetPeek() {
	pt = tokens;
}
