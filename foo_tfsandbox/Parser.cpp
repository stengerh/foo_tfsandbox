
#include "stdafx.h"
#include <wchar.h>
#include "Parser.h"
#include "Scanner.h"




void Parser::SynErr(int n) {
	if (errDist >= minErrDist) errors->SynErr(la->line, la->col, n);
	errDist = 0;
}

void Parser::SemErr(const wchar_t* msg) {
	if (errDist >= minErrDist) errors->Error(t->line, t->col, msg);
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;
		la = scanner->Scan();
		if (la->kind <= maxT) { ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::Script() {
		Block(root);
}

void Parser::Block(ast::block_expression *&n) {
		Token *t1 = 0;
		ast::expression *n1 = 0;
		pfc::ptr_list_t<ast::expression> list;
		n = 0; 
		t1 = t; 
		while (StartOf(1)) {
			if (StartOf(2)) {
				Expression(n1);
				list.add_item(n1); 
			} else {
				Error();
			}
		}
		n = new ast::block_expression(t1, list); 
}

void Parser::Expression(ast::expression *&n) {
		if (la->kind == 14) {
			Comment(n);
		} else if (la->kind == 1 || la->kind == 2 || la->kind == 5) {
			ast::string_constant *n1 = 0; 
			String(n1);
			n = n1; 
		} else if (la->kind == 3) {
			ast::field_reference *n2 = 0; 
			Field(n2);
			n = n2; 
		} else if (la->kind == 6) {
			ast::condition_expression *n1 = 0; 
			Condition(n1);
			n = n1; 
		} else if (la->kind == 4) {
			ast::call_expression *n1 = 0; 
			Call(n1);
			n = n1; 
		} else SynErr(16);
}

void Parser::Error() {
		if (la->kind == 11) {
			Get();
			SemErr(L"invalid string syntax"); 
		} else if (la->kind == 12) {
			Get();
			SemErr(L"invalid field syntax"); 
		} else if (la->kind == 13) {
			Get();
			SemErr(L"invalid function syntax"); 
		} else SynErr(17);
}

void Parser::Comment(ast::expression *&n) {
		Expect(14);
		n = new ast::comment(t); 
}

void Parser::String(ast::string_constant *&n) {
		n = 0; 
		if (la->kind == 1) {
			Get();
		} else if (la->kind == 2) {
			Get();
		} else if (la->kind == 5) {
			Get();
		} else SynErr(18);
		n = new ast::string_constant(t); 
}

void Parser::Field(ast::field_reference *&n) {
		n = 0; 
		Expect(3);
		n = new ast::field_reference(t); 
}

void Parser::Condition(ast::condition_expression *&n) {
		ast::block_expression *n1 = 0;
		Token *t1 = 0;
		n = 0; 
		Expect(6);
		t1 = t; 
		Block(n1);
		Expect(7);
		n = new ast::condition_expression(t1, n1, t); 
}

void Parser::Call(ast::call_expression *&n) {
		ast::block_expression *n1;
		Token *tname = 0;
		pfc::ptr_list_t<Token> tdelims;
		pfc::ptr_list_t<ast::block_expression> params;
		n = 0; 
		Expect(4);
		tname = t; 
		Expect(8);
		tdelims.add_item(t); 
		Block(n1);
		params.add_item(n1); 
		while (la->kind == 10) {
			Get();
			tdelims.add_item(t); 
			Block(n1);
			params.add_item(n1); 
		}
		Expect(9);
		tdelims.add_item(t); n = new ast::call_expression(tname, tdelims, params); 
}



void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create("Dummy Token");
	Get();
	Script();

	Expect(0);
}

Parser::Parser(Scanner *scanner, ErrorOutput *output) {
	maxT = 15;

	dummyToken = NULL;
	t = la = NULL;
	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
	this->errors = new Errors(output);
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[3][17] = {
		{T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,T,T,T, T,T,T,x, x,x,x,T, T,T,T,x, x},
		{x,T,T,T, T,T,T,x, x,x,x,x, x,x,T,x, x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	delete dummyToken;
}

Errors::Errors(ErrorOutput *output) {
	count = 0;
	this->output = output;
}

void Errors::SynErr(int line, int col, int n) {
	wchar_t* s;
	switch (n) {
			case 0: s = coco_string_create(L"EOF expected"); break;
			case 1: s = coco_string_create(L"literal expected"); break;
			case 2: s = coco_string_create(L"string expected"); break;
			case 3: s = coco_string_create(L"field expected"); break;
			case 4: s = coco_string_create(L"func expected"); break;
			case 5: s = coco_string_create(L"selfquot expected"); break;
			case 6: s = coco_string_create(L"\"[\" expected"); break;
			case 7: s = coco_string_create(L"\"]\" expected"); break;
			case 8: s = coco_string_create(L"\"(\" expected"); break;
			case 9: s = coco_string_create(L"\")\" expected"); break;
			case 10: s = coco_string_create(L"\",\" expected"); break;
			case 11: s = coco_string_create(L"badstring expected"); break;
			case 12: s = coco_string_create(L"badfield expected"); break;
			case 13: s = coco_string_create(L"badfunc expected"); break;
			case 14: s = coco_string_create(L"comment expected"); break;
			case 15: s = coco_string_create(L"??? expected"); break;
			case 16: s = coco_string_create(L"invalid Expression"); break;
			case 17: s = coco_string_create(L"invalid Error"); break;
			case 18: s = coco_string_create(L"invalid String"); break;

		default:
		{
			wchar_t format[20];
			coco_swprintf(format, 20, L"error %d", n);
			s = coco_string_create(format);
		}
		break;
	}
	Print(line, col, s);
	coco_string_delete(s);
	count++;
}

void Errors::Error(int line, int col, const wchar_t *s) {
	Print(line, col, s);
	count++;
}

void Errors::Warning(int line, int col, const wchar_t *s) {
	Print(line, col, s);
}

void Errors::Warning(const wchar_t *s) {
	Print(s);
}

void Errors::Exception(const wchar_t* s) {
	throw std::exception(pfc::stringcvt::string_utf8_from_wide(s).get_ptr());
}

void Errors::Print(int line, int col, const wchar_t *s) {
	if (output != 0) output->Print(line, col, s);
}

void Errors::Print(const wchar_t *s) {
	if (output != 0) output->Print(s);
}



