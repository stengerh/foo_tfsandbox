#include "stdafx.h"
#include "titleformat_debug.h"
#include "titleformat_analysis.h"

#define U2W(Text) pfc::stringcvt::string_wide_from_utf8(Text).get_ptr()
#define W2U(Text) pfc::stringcvt::string_utf8_from_wide(Text).get_ptr()

void titleformat_debugger_environment::reset()
{
	m_trace.remove_all();
	m_node_data_heap.delete_all();
	m_variable_data_heap.delete_all();
}

titleformat_debugger_environment::node_data *titleformat_debugger_environment::find_node_data(ast::node *n) const
{
	node_data *data = 0;
	if (m_trace.query(n, data)) return data;
	return 0;
}

titleformat_debugger_environment::node_data *titleformat_debugger_environment::create_node_data(ast::node *n, titleformat_debugger_environment::variable_data *variables)
{
	node_data *data = new node_data(variables);
	m_trace.set(n, data);
	m_node_data_heap.add_item(data);
	return data;
}

bool titleformat_debugger_environment::lookup_variable(const titleformat_debugger_environment::node_data *data, const char *p_name, pfc::string &p_out)
{
	variable_data *var = data->m_variables;

	while (var != 0)
	{
		if (stricmp_utf8(var->m_name, p_name) == 0)
		{
			p_out = var->m_string_value;
			return true;
		}
		var = var->m_parent;
	}
	return false;
}

titleformat_debugger_environment::variable_data * titleformat_debugger_environment::set_variable(node_data *data, const char *p_name, pfc::string p_string_value)
{
	variable_data *var = new variable_data();
	var->m_parent = data->m_variables;
	data->m_variables = var;
	var->m_name = p_name;
	var->m_string_value = p_string_value;
	return var;
}

titleformat_debugger::titleformat_debugger()
{
	m_parser_error_count = 0;

	m_trace_valid = false;
}

void titleformat_debugger::reset()
{
	m_env.reset();
	m_trace_valid = false;

	m_analysis.reset();

	m_script.reset();
}

bool titleformat_debugger::parse(const char *p_format)
{
	reset();
	m_parser_error_count = m_script.parse(p_format, strlen(p_format), m_parser_messages);
	return m_parser_error_count == 0;
}

bool titleformat_debugger::get_value(ast::node *n, pfc::string_base &p_string_value, bool &p_bool_value)
{
	p_string_value = "";
	p_bool_value = false;

	if (m_trace_valid)
	{
		titleformat_debugger_environment::node_data *data = m_env.find_node_data(n);
		if (data != 0)
		{
			p_string_value = data->m_string_value.get_ptr();
			p_bool_value = data->m_bool_value;
			return true;
		}
	}

	return false;
}

bool titleformat_debugger::test_value(ast::node *n)
{
	if (m_trace_valid)
	{
		titleformat_debugger_environment::node_data *data = m_env.find_node_data(n);
		if (data != 0)
		{
			return true;
		}
	}

	return false;
}

bool titleformat_debugger::is_function_unknown(ast::call_expression *n)
{
	if (m_trace_valid)
	{
		return m_analysis.is_function_unknown(n->get_function_name(), n->get_param_count());
	}

	return n->kind() == ast::node::kind_call;;
}


class visitor_titleformat_debug : public ast::visitor
{
	titleformat_debugger_environment &m_env;
	metadb_handle_ptr m_handle;
	static_api_ptr_t<titleformat_compiler> tfc;
	titleformat_debugger_environment::variable_data *m_variables;
	titleformat_analysis &m_analysis;

public:
	visitor_titleformat_debug(titleformat_debugger_environment &p_env, titleformat_analysis &p_analysis, metadb_handle_ptr p_handle) : m_env(p_env), m_analysis(p_analysis), m_handle(p_handle),  m_variables(0) {}

	void compute_value(ast::node *n);

	virtual void visit(ast::comment *n);
	virtual void visit(ast::string_constant *n);
	virtual void visit(ast::field_reference *n);
	virtual void visit(ast::call_expression *n);
	virtual void visit(ast::condition_expression *n);
	virtual void visit(ast::block_expression *n);
};

void titleformat_debugger::trace(metadb_handle_ptr p_handle)
{
	m_analysis.init(p_handle, get_root());

    if (p_handle.is_valid())
    {
	    visitor_titleformat_debug v(m_env, m_analysis, p_handle);
	    v.compute_value(get_root());
    }

	m_trace_valid = true;
}

void visitor_titleformat_debug::compute_value(ast::node *n)
{
	n->accept(this);
}

void visitor_titleformat_debug::visit(ast::comment *n)
{
	// do not evaluate
}

void visitor_titleformat_debug::visit(ast::string_constant *n)
{
	titleformat_debugger_environment::node_data *data = m_env.create_node_data(n, m_variables);
	data->m_bool_value = false;

	const char *ptr = n->get_text().get_ptr();
	if ((ptr[0] == '\'' && ptr[1] == '\'') || ptr[0] == '%' || ptr[0] == '$')
		data->m_string_value = pfc::string(ptr, 1);
	else if (ptr[0] == '\'')
		data->m_string_value = pfc::string(ptr + 1, strlen(ptr) - 2);
	else
		data->m_string_value = pfc::string(ptr);
}

void visitor_titleformat_debug::visit(ast::field_reference *n)
{
	titleformat_debugger_environment::node_data *data = m_env.create_node_data(n, m_variables);
	data->m_string_value = "";
	data->m_bool_value = false;

	pfc::string_formatter buffer;
#if 0
	pfc::string_formatter format_string, format_bool;

	format_string << W2U(n->value);
	format_bool << "[1" << format_string << "]";

	titleformat_object::ptr script;
	if (tfc->compile(script, format_string))
	{
		buffer.reset();
		m_handle->format_title(0, buffer, script, 0);
		data->m_string_value = buffer;
	}
	if (tfc->compile(script, format_bool))
	{
		buffer.reset();
		m_handle->format_title(0, buffer, script, 0);
		data->m_bool_value = buffer.length() > 0;
	}
#else
	pfc::string_formatter format;

	format << "$if($puts(.," << n->get_field_name().get_ptr() << "),1,0)$get(.)";

	titleformat_object::ptr script;
	if (tfc->compile(script, format))
	{
		buffer.reset();
        if (m_handle.is_valid())
        {
		    m_handle->format_title(nullptr, buffer, script, nullptr);
		    data->m_string_value = &buffer[1];
		    data->m_bool_value = buffer[0] == '1';
        }
	}
#endif
}

class titleformat_hook_impl_call_expression : public titleformat_hook
{
	visitor_titleformat_debug *v;
	titleformat_debugger_environment &m_env;
	ast::call_expression *n;

public:
	titleformat_hook_impl_call_expression(visitor_titleformat_debug *_v, titleformat_debugger_environment &_env, ast::call_expression *_n) : v(_v), m_env(_env), n(_n)
	{
	}

	virtual bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag)
	{
		return false;
	}

	virtual bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
	{
		if (pfc::strcmp_ex(p_name, p_name_length, "__trace", 7) == 0)
		{
			if (p_params->get_param_count() == 1)
			{
				p_found_flag = false;
				t_size index = p_params->get_param_uint(0);

				if (index < n->get_param_count())
				{
					ast::block_expression *child = n->get_param(index);
					child->accept(v);
					titleformat_debugger_environment::node_data *child_data = m_env.find_node_data(child);

					if (child_data)
					{
						p_out->write(titleformat_inputtypes::unknown, child_data->m_string_value.get_ptr());
						p_found_flag = child_data->m_bool_value;
					}
				}
			}
			return true;
		}
		return false;
	}
};

void visitor_titleformat_debug::visit(ast::call_expression *n)
{
	t_size param_count = n->get_param_count();
	pfc::string ident = n->get_function_name();
	const char* cname = ident.get_ptr() + 1;

	if (stricmp_utf8(cname, "get") == 0)
	{
		if (param_count == 1)
		{
			ast::block_expression *param = n->get_param(0);
			param->accept(this);
			titleformat_debugger_environment::node_data *param_data = m_env.find_node_data(param);

			titleformat_debugger_environment::node_data *data = m_env.create_node_data(n, m_variables);

			if (param_data)
			{
				data->m_bool_value = m_env.lookup_variable(data, param_data->m_string_value.get_ptr(), data->m_string_value);
			}
		}
	}
	else if (stricmp_utf8(cname, "put") == 0 || stricmp_utf8(cname, "puts") == 0)
	{
		if (param_count == 2)
		{
			ast::block_expression *param_name = n->get_param(0);
			param_name->accept(this);
			titleformat_debugger_environment::node_data *param_name_data = m_env.find_node_data(param_name);

			ast::block_expression *param_value = n->get_param(1);
			param_value->accept(this);
			titleformat_debugger_environment::node_data *param_value_data = m_env.find_node_data(param_value);

			titleformat_debugger_environment::node_data *data = m_env.create_node_data(n, m_variables);

			if (param_value_data && param_name_data)
			{
				m_variables = m_env.set_variable(data, param_name_data->m_string_value.get_ptr(), param_value_data->m_string_value);
				if (stricmp_utf8(cname, "put") == 0) data->m_string_value = param_value_data->m_string_value;
				data->m_bool_value = param_value_data->m_bool_value;
			}
		}
	}
	else if (m_analysis.is_function_unknown(n->get_function_name(), n->get_param_count()))
	{
			titleformat_debugger_environment::node_data *data = m_env.create_node_data(n, m_variables);
			data->m_string_value = "[UNKNOWN FUNCTION]";
			data->m_bool_value = false;
			data->m_unknown_function = true;
	}
	else
	{
		pfc::string_formatter format;

		format << "$if($puts(.,$" << cname << "(";
		for (t_size param_index = 0; param_index < param_count; ++param_index)
		{
			if (param_index > 0) format << ",";
			format << "$__trace(" << param_index << ")";
		}
		format << ")),1,0)$get(.)";

		titleformat_object::ptr script;
		if (tfc->compile(script, format))
		{
			titleformat_debugger_environment::node_data *data = m_env.create_node_data(n, m_variables);

			pfc::string_formatter buffer;
			m_handle->format_title(&titleformat_hook_impl_call_expression(this, m_env, n), buffer, script, 0);
			data->m_string_value = &buffer[1];
			data->m_bool_value = buffer[0] == '1';
			data->m_variables = m_variables;
		}
	}
}

void visitor_titleformat_debug::visit(ast::condition_expression *n)
{
	ast::node *child = n->get_child(1);
	child->accept(this);
	titleformat_debugger_environment::node_data *childdata = m_env.find_node_data(child);

	titleformat_debugger_environment::node_data *data = m_env.create_node_data(n, m_variables);
	if (childdata && childdata->m_bool_value)
	{
		data->m_string_value = childdata->m_string_value;
		data->m_bool_value = true;
	}
	else
	{
		data->m_string_value = "";
		data->m_bool_value = false;
	}
}

void visitor_titleformat_debug::visit(ast::block_expression *n)
{
	pfc::string_formatter buffer;
	bool flag = false;

	t_size count = n->get_child_count();
	for (t_size index = 0; index < count; ++index)
	{
		ast::node *child = n->get_child(index);
		child->accept(this);
		titleformat_debugger_environment::node_data *data = m_env.find_node_data(child);
		if (data) {buffer << data->m_string_value.get_ptr(); flag |= data->m_bool_value;}
	}

	titleformat_debugger_environment::node_data *data = m_env.create_node_data(n, m_variables);
	data->m_string_value = buffer;
	data->m_bool_value = flag;
}
