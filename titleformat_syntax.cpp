#include "stdafx.h"
#include "titleformat_syntax.h"
#include "Scanner.h"
#include "Parser.h"

class ErrorOutputImpl : public ErrorOutput
{
	pfc::string_base & m_out;

public:
	ErrorOutputImpl(pfc::string_base & p_out) : m_out(p_out) {}

	virtual void Print(int line, int col, const wchar_t *s)
	{
		m_out << "-- line " << line << ", colum " << col << ": " << pfc::stringcvt::string_utf8_from_wide(s) << "\r\n";
	}

	virtual void Print(const wchar_t *s)
	{
		m_out << pfc::stringcvt::string_utf8_from_wide(s) << "\r\n";
	}
};

namespace ast
{
	/////////////////////////////////////////////////////////
	// class block_expression

	block_expression::block_expression(Token *t, pfc::ptr_list_t<ast::expression> &list) : node(kind_block)
	{
		position pos = {-1, -1, -1, -1};
		m_pos = pos;

		const t_size count = list.get_count();
		m_expressions.set_size(count);
		for (t_size index = 0; index < count; ++index) m_expressions[index] = list[index];

		if (count > 0)
		{
			m_pos = m_expressions[0]->get_position();
			m_pos.end = m_expressions[count-1]->get_end();
		}
	}

	t_size block_expression::get_child_count() const {return m_expressions.get_count();}
	node *block_expression::get_child(t_size index) const {return m_expressions[index];}

	void block_expression::get_source_recur(pfc::string_receiver &source, int &line) const
	{
		for (t_size index = 0; index < get_child_count(); ++index)
		{
			get_child(index)->get_source_recur(source, line);
		}
	}

	void block_expression::accept(visitor *v) {v->visit(this);}

	t_size block_expression::get_expression_count() const {return m_expressions.get_count();}
	expression *block_expression::get_expression(t_size index) const {return m_expressions[index];}

	/////////////////////////////////////////////////////////
	// class comment

	comment::comment(Token *t) : expression(kind_comment)
	{
		m_pos.init_from_token(t);
		m_text = pfc::string(const_cast<const char *>(t->val));
	}

	t_size comment::get_child_count() const {return 0;}
	node *comment::get_child(t_size index) const {pfc::dynamic_assert(false); return 0;}

	void comment::get_source_recur(pfc::string_receiver &source, int &line) const
	{
		get_source_newline(source, line, m_pos.line);
		source.add_string(get_text().get_ptr());
	}

	void comment::accept(visitor *v) {v->visit(this);}

	pfc::string comment::get_text() const {return m_text;}

	/////////////////////////////////////////////////////////
	// class field_reference

	field_reference::field_reference(Token *t) : expression(kind_field)
	{
		m_pos.init_from_token(t);
		m_field_name = pfc::string(const_cast<const char *>(t->val));
	}

	t_size field_reference::get_child_count() const {return 0;}
	node *field_reference::get_child(t_size index) const {pfc::dynamic_assert(false); return 0;}

	void field_reference::get_source_recur(pfc::string_receiver &source, int &line) const
	{
		if (m_pos.line != line) {source.add_string("\r\n"); line = m_pos.line;}
		source.add_string(get_field_name().get_ptr());
	}

	void field_reference::accept(visitor *v) {v->visit(this);}

	pfc::string field_reference::get_field_name() const
	{
		return m_field_name;
	}

	/////////////////////////////////////////////////////////
	// class condition_expression

	condition_expression::condition_expression(Token *open, block_expression *param, Token *close) : expression(kind_condition)
	{
		m_pos_open.init_from_token(open);
		m_pos_close.init_from_token(close);
		m_pos = m_pos_open;
		m_pos.end = m_pos_close.end;
		m_param = param;
	}

	t_size condition_expression::get_child_count() const {return 1;}
	node *condition_expression::get_child(t_size index) const {return m_param;}

	void condition_expression::get_source_recur(pfc::string_receiver &source, int &line) const
	{
		get_source_newline(source, line, m_pos_open.line);
		source.add_string("[");

		get_param()->get_source_recur(source, line);

		get_source_newline(source, line, m_pos_close.line);
		source.add_string("]");
	}

	void condition_expression::accept(visitor *v) {v->visit(this);}

	block_expression *condition_expression::get_param() const {return m_param;}

	/////////////////////////////////////////////////////////
	// class string_constant
		
	string_constant::string_constant(Token *t) : expression(kind_string)
	{
		m_pos.init_from_token(t);
		m_text = pfc::string(const_cast<const char *>(t->val));
	}

	t_size string_constant::get_child_count() const {return 0;}
	node *string_constant::get_child(t_size index) const {pfc::dynamic_assert(false); return 0;}

	void string_constant::get_source_recur(pfc::string_receiver &source, int &line) const
	{
		get_source_newline(source, line, m_pos.line);
		source.add_string(get_text().get_ptr());
	}

	void string_constant::accept(visitor *v) {v->visit(this);}

	pfc::string string_constant::get_text() const {return m_text;}

	/////////////////////////////////////////////////////////
	// class call_expression
		
	call_expression::call_expression(Token *tname, pfc::list_base_const_t<Token *> const &tdelims, pfc::list_base_const_t<block_expression *> const &params) : expression(kind_call)
	{
		m_name = pfc::string(const_cast<const char *>(tname->val));
		m_pos_name.init_from_token(tname);
		
		m_pos_delims.set_size(tdelims.get_count());
		for (t_size index = 0; index < tdelims.get_count(); ++index) m_pos_delims[index].init_from_token(tdelims[index]);

		m_params.set_size(params.get_count());
		for (t_size index = 0; index < params.get_count(); ++index) m_params[index] = params[index];

		m_pos = m_pos_name;
		m_pos.end = m_pos_delims[m_pos_delims.get_count() - 1].end;
	}

	t_size call_expression::get_child_count() const {return m_params.get_count();}

	node *call_expression::get_child(t_size index) const {return m_params[index];}

	void call_expression::get_source_recur(pfc::string_receiver &source, int &line) const
	{
		get_source_newline(source, line, m_pos_name.line);
		source.add_string(get_function_name().get_ptr());

		const t_size count = get_child_count();
		for (t_size index = 0; index < count; ++index)
		{
			get_source_newline(source, line, m_pos_delims[0].line);
			source.add_string((index == 0) ? "(" : ",");

			get_child(index)->get_source_recur(source, line);
		}

		get_source_newline(source, line, m_pos_delims[count].line);
		source.add_string(")");
	}

	void call_expression::accept(visitor *v) {v->visit(this);}

	pfc::string call_expression::get_function_name() const {return m_name;}

	t_size call_expression::get_param_count() const
	{
		t_size count = m_params.get_count();
		if (count == 1 && m_params[0]->get_child_count() == 0 && m_pos_delims[0].line < m_pos_delims[1].line) count = 0;
		return count;
	}

	block_expression *call_expression::get_param(t_size index) const {return m_params[index];}




	void sequence::get_source(pfc::string_receiver &source) const
	{
		int line = -1;
		for (t_size index = 0; index < get_count(); ++index)
		{
			get_item(index)->get_source_recur(source, line);
		}
	}

	script::script() : root(0) {}
	script::~script()
	{
		reset();
	}

	int script::parse(const char *source, t_size length, pfc::string_base &errors)
	{
		reset();
		errors = "";
		Scanner scanner((const unsigned char *)source, length);
		ErrorOutputImpl output(errors);
		Parser parser(&scanner, &output);
		parser.Parse();
		this->root = parser.root;
		return parser.errors->count;
	}

	class visitor_find_fragment : private visitor
	{
		// Recursion into a node n happens if and only if n is known to contain the selection.
		// The found flag is only updated in visit(block_expression *).
		// All other visit() overloads just recurse further, if the selection is contained in one
		// of their block_expression children.
	public:
		bool run(fragment &out, block_expression *root, int _start, int _end, node_filter *_filter)
		{
			start = _start;
			end = _end;
			filter = _filter;
			found = false;
			result = fragment(root, 0, 0);

			root->accept(this);

			out = result;
			return result.first_child < result.last_child;
		}

	private:
		// input parameters
		int start;
		int end;
		node_filter *filter;
		// output parameters
		fragment result;
		// flag
		bool found;

		void visit(comment *n) {}
		void visit(string_constant *n) {}
		void visit(field_reference *n) {}

		void visit(condition_expression *e)
		{
			if (!filter || filter->test(e))
			{
				position pos = e->get_position();

				if (start > pos.start && end < pos.end)
				{
					e->get_param()->accept(this);
				}
			}
		}

		void visit(call_expression *e)
		{
			if (!filter || filter->test(e))
			{
				// not get_param_count(), we want the first parameter, even if it is empty.
				const t_size count = e->get_child_count();
				for (t_size index = 0; index < count; ++index)
				{
					int before = e->get_delim_position(index).end;
					int after = e->get_delim_position(index+1).start;

					if (start >= before && end <= after)
					{
						e->get_child(index)->accept(this);
						break;
					}
				}
			}
		}

		void visit(block_expression *b)
		{
			const t_size count = b->get_expression_count();
			for (t_size index = 0; index < count; ++index)
			{
				expression *e = b->get_expression(index);
				position pos = e->get_position();

				// between expressions or before first expression
				if (end < pos.start)
				{
					found = true;
					result = fragment(b, index, index);
					break;
				}

				if (start >= pos.start)
				{
					// inside expression
					if (end <= pos.end)
					{
						e->accept(this);
						if (!found)
						{
							found = true;
							result = fragment(b, index, index+1);
						}
						break;
					}
					// overlapping this and a number of following expressions
					else if (start < pos.end)
					{
						t_size index2 = index;
						for (; index2 < count; ++index2)
						{
							e = b->get_expression(index2);
							pos = e->get_position();

							// selection ends before expression
							if (end <= pos.start)
							{
								break;
							}
						}
						found = true;
						result = fragment(b, index, index2);
						break;
					}
				}
			}

			// after last expression
			if (!found)
			{
				found = true;
				result = fragment(b, count, count);
			}
		}
	};

	bool find_fragment(fragment &out, block_expression *root, int start, int end, node_filter *filter)
	{
		position pos = root->get_position();
		const t_size count = root->get_child_count();

		if (end < pos.start)
		{
			out = fragment(root, 0, 0);
			return false;
		}

		if (start > pos.end)
		{
			out = fragment(root, count, count);
			return false;
		}

		if (start <= pos.start && end >= pos.end)
		{
			out = fragment(root, 0, count);
			return true;
		}

		return visitor_find_fragment().run(out, root, pfc::min_t(start, end), pfc::max_t(start, end), filter);
	}
} // namespace
