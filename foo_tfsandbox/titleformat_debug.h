#pragma once

#include "titleformat_syntax.h"
#include "titleformat_analysis.h"

class titleformat_debugger_environment
{
public:
	struct variable_data
	{
		variable_data *m_parent;
		pfc::string8 m_name;
		pfc::string m_string_value;

		variable_data() : m_parent(0) {}
	};

	struct node_data
	{
		pfc::string m_string_value;
		bool m_bool_value;
		bool m_unknown_function;
		variable_data *m_variables;

		node_data(variable_data *variables) : m_bool_value(false), m_unknown_function(false), m_variables(variables) {}
	};

	pfc::map_t<ast::node *, node_data *> m_trace;
	pfc::ptr_list_t<node_data> m_node_data_heap;
	pfc::ptr_list_t<variable_data> m_variable_data_heap;

	node_data *find_node_data(ast::node *n) const;
	node_data *create_node_data(ast::node *n, variable_data *variables);

	bool lookup_variable(const node_data *data, const char *p_name, pfc::string &p_out);
	variable_data *set_variable(node_data *data, const char *p_name, pfc::string p_string_value);

	void reset();
};

class titleformat_debugger
{
	ast::script m_script;
	int m_parser_error_count;
	pfc::string_formatter m_parser_messages;

	titleformat_debugger_environment m_env;
	bool m_trace_valid;

	titleformat_analysis m_analysis;

public:
	titleformat_debugger();
	~titleformat_debugger() {reset();}

	void reset();

	bool parse(const char *p_format);
	void trace(metadb_handle_ptr p_handle);

	int get_parser_errors() const {return m_parser_error_count;}
	int get_parser_errors(pfc::string_base &out) const {out = m_parser_messages; return m_parser_error_count;}

	ast::block_expression *get_root() const {return m_script.get_root();}

	bool get_value(ast::node *n, pfc::string_base &p_string_value, bool &p_bool_value);
	bool test_value(ast::node *n);
	bool is_function_unknown(ast::call_expression *n);
};
