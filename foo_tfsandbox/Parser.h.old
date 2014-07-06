

#if !defined(COCO_PARSER_H__)
#define COCO_PARSER_H__

#include "titleformat_syntax.h"


#include "Scanner.h"



class ErrorOutput {
public:
	virtual void Print(int line, int col, const wchar_t *s) {}
	virtual void Print(const wchar_t *s) {}
};

class Errors {
public:
	int count;			// number of errors detected
	ErrorOutput *output;

	Errors(ErrorOutput *output = 0);
	void SynErr(int line, int col, int n);
	void Error(int line, int col, const wchar_t *s);
	void Warning(int line, int col, const wchar_t *s);
	void Warning(const wchar_t *s);
	void Exception(const wchar_t *s);
	
	void Print(int line, int col, const wchar_t *s);
	void Print(const wchar_t *s);
}; // Errors

class Parser {
private:
	enum {
		_EOF=0,
		_literal=1,
		_string=2,
		_field=3,
		_func=4,
		_selfquot=5,
		_badstring=11,
		_badfield=12,
		_badfunc=13,
		_comment=14,
	};
	int maxT;

	Token *dummyToken;
	int errDist;
	int minErrDist;

	void SynErr(int n);
	void Get();
	void Expect(int n);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);

public:
	Scanner *scanner;
	Errors  *errors;

	Token *t;			// last recognized token
	Token *la;			// lookahead token

ast::block_expression *root;

/*
CHARACTERS
	funcchar = ANY - '\r' - '\n' - '('.
	funcchar1 = funcchar - '$'.
	stringchar = ANY - '\''.
	fieldchar = ANY - '%'.
	literalchar = ANY - '\r' - '\n' - '(' - ')' - ',' - '$' - '\'' - '%' - '[' - ']'.
	
TOKENS
	literal = literalchar {literalchar}.
	string = '\'' stringchar {stringchar} '\''.
	field = '%' fieldchar {fieldchar} '%'.
	func = '$' funcchar1 {funcchar}.
	selfquot = "$$" | "''" | "%%".
	"["
	"]"
	"("
	")"
	","
	
COMMENTS FROM "//" TO "\n"
	
IGNORE '\r' + '\n'
*/



	Parser(Scanner *scanner, ErrorOutput *output = 0);
	~Parser();
	void SemErr(const wchar_t* msg);

	void Script();
	void Block(ast::block_expression *&n);
	void Expression(ast::expression *&n);
	void Error();
	void Comment(ast::expression *&n);
	void String(ast::string_constant *&n);
	void Field(ast::field_reference *&n);
	void Condition(ast::condition_expression *&n);
	void Call(ast::call_expression *&n);

	void Parse();

}; // end Parser



#endif // !defined(COCO_PARSER_H__)

