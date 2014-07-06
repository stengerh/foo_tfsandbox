#pragma once

#include "Scanner.h"

namespace ast
{
	class node;
	class expression;
	class block_expression;
	class comment;
	class string_constant;
	class field_reference;
	class call_expression;
	class condition_expression;

	class visitor
	{
	public:
		virtual void visit(block_expression *n) = 0;
		virtual void visit(comment *n) = 0;
		virtual void visit(string_constant *n) = 0;
		virtual void visit(field_reference *n) = 0;
		virtual void visit(condition_expression *n) = 0;
		virtual void visit(call_expression *n) = 0;
	};

	class sequence : public pfc::ptr_list_t<node>
	{
	public:
		void get_source(pfc::string_receiver &source) const;
	};

	struct position
	{
		int start;
		int end;

		int line;
		int column;

		void init_from_token(Token *t)
		{
			start = t->pos;
			end = start + strlen(t->val);
			line = t->line;
			column = t->col;
		}
	};

	class node
	{
	public:
		enum kind_e {
			kind_block,
			kind_comment,
			kind_string,
			kind_field,
			kind_condition,
			kind_call,
		};

		node(kind_e kind) : m_kind(kind) {}
		virtual ~node() {}

		inline kind_e kind() const {return m_kind;}

		virtual t_size get_child_count() const = 0;
		virtual node *get_child(t_size index) const = 0;

		virtual void get_source_recur(pfc::string_receiver &source, int &line) const = 0;

		virtual void accept(visitor *v) = 0;

		inline const position &get_position() const {return m_pos;}
		inline int get_start() const {return get_position().start;}
		inline int get_end() const {return get_position().end;}
		inline int get_line() const {return get_position().line;}
		inline int get_column() const {return get_position().column;}

		inline void get_source(pfc::string_receiver &source) const {int line = -1; get_source_recur(source, line);}
		static inline void get_source_newline(pfc::string_receiver &source, int &line, int newline) {if (newline > line) {source.add_string("\r\n"); line = newline;}}

	private:
		const kind_e m_kind;

	protected:
		position m_pos;
	};

	class block_expression : public node
	{
	public:
		block_expression(Token *t, pfc::ptr_list_t<ast::expression> &list);

		virtual t_size get_child_count() const;
		virtual node *get_child(t_size index) const;

		virtual void get_source_recur(pfc::string_receiver &source, int &line) const;

		virtual void accept(visitor *v);

		t_size get_expression_count() const;
		expression *get_expression(t_size index) const;

	private:
		pfc::array_t<expression *> m_expressions;
	};

	class expression : public node
	{
	public:
		expression(kind_e kind) : node(kind) {}
	};

	class comment : public expression
	{
	public:
		comment(Token *t);

		virtual t_size get_child_count() const;
		virtual node *get_child(t_size index) const;

		virtual void get_source_recur(pfc::string_receiver &source, int &line) const;

		virtual void accept(visitor *v);

		pfc::string get_text() const;

	private:
		pfc::string m_text;
	};

	class string_constant : public expression
	{
	public:
		string_constant(Token *t);

		virtual t_size get_child_count() const;
		virtual node *get_child(t_size index) const;

		virtual void get_source_recur(pfc::string_receiver &source, int &line) const;

		virtual void accept(visitor *v);

		pfc::string get_text() const;

	private:
		pfc::string m_text;
	};

	class field_reference : public expression
	{
	public:
		field_reference(Token *t);

		virtual t_size get_child_count() const;
		virtual node *get_child(t_size index) const;

		virtual void get_source_recur(pfc::string_receiver &source, int &line) const;

		virtual void accept(visitor *v);

		pfc::string get_field_name() const;

	private:
		pfc::string m_field_name;
	};

	class condition_expression : public expression
	{
	public:
		condition_expression(Token *open, block_expression *param, Token *close);

		virtual t_size get_child_count() const;
		virtual node *get_child(t_size index) const;

		virtual void get_source_recur(pfc::string_receiver &source, int &line) const;

		virtual void accept(visitor *v);

		block_expression *get_param() const;

		const position &get_open_position() const {return m_pos_open;}
		const position &get_close_position() const {return m_pos_close;}

	private:
		position m_pos_open;
		position m_pos_close;
		block_expression *m_param;
	};

	class call_expression : public expression
	{
	public:
		call_expression(Token *tname, pfc::list_base_const_t<Token *> const &tdelims, pfc::list_base_const_t<block_expression *> const &params);

		virtual t_size get_child_count() const;
		virtual node *get_child(t_size index) const;

		virtual void get_source_recur(pfc::string_receiver &source, int &line) const;

		virtual void accept(visitor *v);

		pfc::string get_function_name() const;

		t_size get_param_count() const;
		block_expression *get_param(t_size index) const;

		const position &get_delim_position(t_size index) const {return m_pos_delims[index];}

	private:
		position m_pos_name;
		pfc::array_t<position> m_pos_delims;
		pfc::string m_name;
		pfc::array_t<block_expression *> m_params;
	};

	class script
	{
		block_expression *root;

	public:
		script();
		~script();

		block_expression *get_root() const {return root;}
		bool is_valid() const {return root != 0;}

		int parse(const char *source, t_size length, pfc::string_base &errors);

		void reset() {if (root != 0) {delete root; root = 0;}}
		block_expression *detach() {block_expression *n = root; root = 0; return n;}
		void attach(block_expression *n) {if (n != root) {reset(); root = n;}}

		operator block_expression*() {return root;}
	};

	// The fragment contains all children of root with index >= first_child && index < last_child.
	struct fragment
	{
		block_expression *root;
		t_size first_child;
		t_size last_child;

		fragment() : root(0), first_child(0), last_child(0) {}
		fragment(block_expression *_root, t_size _first_child, t_size _last_child) : root(_root), first_child(_first_child), last_child(_last_child) {}

		inline t_size get_count() const {return last_child - first_child;}
		inline expression *operator[](t_size index) const {return root->get_expression(index + first_child);}

		inline bool operator==(const fragment &other) const {return root == other.root && first_child == other.first_child && last_child == other.last_child;}
		inline bool operator!=(const fragment &other) const {return root != other.root || first_child != other.first_child || last_child != other.last_child;}
	};

	class node_filter
	{
	protected:
		node_filter() {}
		~node_filter() {}

	public:
		virtual bool test(block_expression *b) = 0;
		virtual bool test(comment *n) = 0;
		virtual bool test(string_constant *n) = 0;
		virtual bool test(field_reference *n) = 0;
		virtual bool test(condition_expression *n) = 0;
		virtual bool test(call_expression *n) = 0;
	};

	bool find_fragment(fragment &out, block_expression *root, int start, int end, node_filter *filter = 0);
	inline bool find_fragment(fragment &out, block_expression *root, int pos, node_filter *filter = 0) {return find_fragment(out, root, pos, pos, filter);}
} // namespace
