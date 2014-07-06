#include "stdafx.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "atlscilexer.h"
#include "TFDbgDialog.h"

#ifndef U2T
#define U2T(Text) pfc::stringcvt::string_os_from_utf8(Text).get_ptr()
#endif

//bool Scintilla_RegisterClasses(void *hInstance);
//bool Scintilla_ReleaseResources();

// {B2E1B41E-CF32-41b2-884A-AE12D258FE71}
static const GUID guid_cfg_format = { 0xb2e1b41e, 0xcf32, 0x41b2, { 0x88, 0x4a, 0xae, 0x12, 0xd2, 0x58, 0xfe, 0x71 } };
// {2D33BBC8-7D6A-4a5e-9998-6ED8274F52C6}
static const GUID guid_cfg_window_position = { 0x2d33bbc8, 0x7d6a, 0x4a5e, { 0x99, 0x98, 0x6e, 0xd8, 0x27, 0x4f, 0x52, 0xc6 } };

static cfgDialogPosition cfg_window_position(guid_cfg_window_position);

static cfg_string cfg_format(guid_cfg_format,
#ifdef _DEBUG
														 "// This is a comment\r\n'test'test$$%%''\r\n$if(%track artist%,'['test'')\r\n// This is another comment\r\n[%comment%]"
#else
														 "// Type your title formatting code here.\r\n// Put the cursor on an expression to see its value.\r\n[%artist% - ]%title%"
#endif
														 );

namespace ast
{
	class node_filter_impl_unknown_function : public node_filter
	{
	public:
		node_filter_impl_unknown_function(titleformat_debugger &p_debugger) : m_debugger(p_debugger) {}

		virtual bool test(block_expression *b) {return true;}
		virtual bool test(comment *n) {return true;}
		virtual bool test(string_constant *n) {return true;}
		virtual bool test(field_reference *n) {return true;}
		virtual bool test(condition_expression *n) {return true;}

		virtual bool test(call_expression *n)
		{
			return !m_debugger.is_function_unknown(n);
		}

	private:
		titleformat_debugger &m_debugger;
	};
 
}

const char *CTitleFormatSandboxDialog::g_pixmap_false =
"/* XPM */\
static const char *false-simple_xpm[] = {\
/* width height num_colors chars_per_pixel */\
\"    16    16        8            1\",\
/* colors */\
\"  c None\",\
\"# c #e00000\",\
\". c #404040\",\
\"a c #000000\",\
\"b c #000000\",\
\"c c #000000\",\
\"d c #000000\",\
\"e c #000000\",\
/* pixels */\
\"                \",\
\"                \",\
\"   ##########   \",\
\"    ##......#.  \",\
\"    ##.      .  \",\
\"    ##.         \",\
\"    ##.   #     \",\
\"    #######.    \",\
\"    ##....#.    \",\
\"    ##.    .    \",\
\"    ##.         \",\
\"    ##.         \",\
\"    ##.         \",\
\"   ####         \",\
\"    ....        \",\
\"                \"\
};\
";

const char *CTitleFormatSandboxDialog::g_pixmap_true =
"/* XPM */\
static const char *true-simple_xpm[] = {\
/* width height num_colors chars_per_pixel */\
\"    16    16        8            1\",\
/* colors */\
\"  c None\",\
\"# c #00c000\",\
\". c #404040\",\
\"a c #000000\",\
\"b c #000000\",\
\"c c #000000\",\
\"d c #000000\",\
\"e c #000000\",\
/* pixels */\
\"                \",\
\"                \",\
\"   ##########   \",\
\"   #...##...#.  \",\
\"   #.  ##.  #.  \",\
\"    .  ##.   .  \",\
\"       ##.      \",\
\"       ##.      \",\
\"       ##.      \",\
\"       ##.      \",\
\"       ##.      \",\
\"       ##.      \",\
\"       ##.      \",\
\"      ####      \",\
\"       ....     \",\
\"                \"\
};\
";

bool CTitleFormatSandboxDialog::find_fragment(ast::fragment &out, int start, int end)
{
	return ast::find_fragment(out, m_debugger.get_root(), start, end, &ast::node_filter_impl_unknown_function(m_debugger));
}

CWindow CTitleFormatSandboxDialog::g_wndInstance;

CTitleFormatSandboxDialog::CTitleFormatSandboxDialog() : m_dlgPosTracker(cfg_window_position)
{
	static bool g_init = false;

	if (!g_init)
	{
		INITCOMMONCONTROLSEX icce;
		icce.dwSize = sizeof(icce);
		icce.dwICC = ICC_TREEVIEW_CLASSES;

		::InitCommonControlsEx(&icce);

		g_init = true;
	}

	m_hSciLexerDll = ::LoadLibrary(TEXT("SciLexer.dll"));
	m_hLexTitleFormatDll = ::LoadLibrary(TEXT("LexTitleformat.dll"));
}

CTitleFormatSandboxDialog::~CTitleFormatSandboxDialog()
{
	if (m_hSciLexerDll != NULL)
	{
			FreeLibrary(m_hSciLexerDll);
	}

	if (m_hLexTitleFormatDll != NULL)
	{
		::FreeLibrary(m_hLexTitleFormatDll);
	}
}

bool CTitleFormatSandboxDialog::pretranslate_message(MSG *pMsg)
{
	if (GetActiveWindow() == *this)
	{
		if (m_accel.TranslateAccelerator(*this, pMsg))
			return true;
	}
	if (IsDialogMessage(pMsg))
		return true;
	return false;
}

void CTitleFormatSandboxDialog::ActivateDialog()
{
	if (!g_wndInstance)
	{
		CTitleFormatSandboxDialog * dlg = new CTitleFormatSandboxDialog();
		if (dlg->m_hSciLexerDll != NULL)
		{
			dlg->Create(core_api::get_main_window());
			dlg->ShowWindow(SW_SHOW);
		}
		else
		{
			console::formatter() << core_api::get_my_file_name() << ": Could not load SciLexer.dll";
			delete dlg;
		}
	}
	else
	{
		g_wndInstance.SetFocus();
	}
}

void CTitleFormatSandboxDialog::SetupTitleFormatStyles(CSciLexerCtrl sciLexer)
{
	sciLexer.Call(SCI_SETMARGINWIDTHN, 1, 0);

	sciLexer.Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
	sciLexer.Call(SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_END);

	sciLexer.StyleSetFont(STYLE_DEFAULT, "Courier New");
	sciLexer.StyleSetFore(STYLE_DEFAULT, RGB(0, 0, 0));
	sciLexer.StyleSetBack(STYLE_DEFAULT, RGB(255, 255, 255));
	sciLexer.StyleClearAll();

	// Comments
	sciLexer.StyleSetFore(1 /*SCE_TITLEFORMAT_COMMENTLINE*/, RGB(0, 128, 0));

	// Operators
	sciLexer.StyleSetBold(2 /*SCE_TITLEFORMAT_OPERATOR*/, true);

	// Fields
	sciLexer.StyleSetFore(3 /*SCE_TITLEFORMAT_FIELD*/, RGB(0, 64, 192));

	// Strings (Single quoted string)
	sciLexer.StyleSetItalic(4 /*SCE_TITLEFORMAT_STRING*/, true);

	// Text (Unquoted string)
	//sciLexer.StyleSetBack(5 /*SCE_TITLEFORMAT_LITERALSTRING*/, RGB(255, 255, 128));

	// Characters (%%, &&, '')
	sciLexer.StyleSetFore(6 /*SCE_TITLEFORMAT_SPECIALSTRING*/, RGB(128, 128, 128));

	// Functions
	sciLexer.StyleSetFore(7 /*SCE_TITLEFORMAT_IDENTIFIER*/, RGB(192, 0, 192));

	sciLexer.MarkerDefinePixmap(0, g_pixmap_false);
	sciLexer.MarkerDefinePixmap(1, g_pixmap_true);

	int lex1 = sciLexer.GetLexer();
	sciLexer.SetLexerLanguage("titleformat1");
	int lex2 = sciLexer.GetLexer();
}

BOOL CTitleFormatSandboxDialog::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_editor.Attach(GetDlgItem(IDC_SCRIPT));
	m_sciLexerFragment.Attach(GetDlgItem(IDC_FRAGMENT));
	m_editStringValue.Attach(GetDlgItem(IDC_STRINGVALUE));
	m_editBoolValue.Attach(GetDlgItem(IDC_BOOLVALUE));
	m_treeScript.Attach(GetDlgItem(IDC_TREE));

	m_editor.SetCodePage(SC_CP_UTF8);
	m_editor.SetModEventMask(SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);
	m_editor.SetMouseDwellTime(500);

	m_sciLexerFragment.SetCodePage(SC_CP_UTF8);
	m_sciLexerFragment.SetModEventMask(0);
	m_sciLexerFragment.Call(SCI_SETREADONLY, true);


	m_editor.CallTipUseStyle(50);

	// Set up styles
	SetupTitleFormatStyles(m_editor);
	SetupTitleFormatStyles(m_sciLexerFragment);

	m_editor.StyleSetFont(STYLE_CALLTIP, "Tahoma");
	m_editor.StyleSetFore(STYLE_CALLTIP, GetSysColor(COLOR_INFOTEXT));
	m_editor.StyleSetBack(STYLE_CALLTIP, GetSysColor(COLOR_INFOBK));

	// inactive code
	m_editor.IndicSetStyle(indicator_inactive_code, INDIC_DIAGONAL);
	m_editor.IndicSetFore(indicator_inactive_code, RGB(64, 64, 64));

	// selected fragment
	m_editor.IndicSetStyle(indicator_fragment, INDIC_ROUNDBOX);
	m_editor.IndicSetFore(indicator_fragment, RGB(64, 128, 255));
	m_editor.IndicSetUnder(indicator_fragment, true);

	// errors
	m_editor.IndicSetStyle(indicator_error, INDIC_SQUIGGLE);
	m_editor.IndicSetFore(indicator_error, RGB(255, 0, 0));

	CImageList imageList;
	imageList.CreateFromImage(IDB_BITMAP1, 16, 2, RGB(255, 0, 255), IMAGE_BITMAP);
	m_treeScript.SetImageList(imageList);

	m_script_update_pending = true;
	m_updating_fragment = false;


	m_editor.SetText(cfg_format);
	m_editor.EmptyUndoBuffer();
	m_editor.SetUndoCollection(true);

	m_accel.LoadAccelerators(IDD);

	static_api_ptr_t<message_loop>()->add_message_filter(this);

	g_wndInstance = *this;

	return FALSE;
}

void CTitleFormatSandboxDialog::OnDestroy()
{
	g_wndInstance = NULL;

	static_api_ptr_t<message_loop>()->remove_message_filter(this);
}

void CTitleFormatSandboxDialog::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DestroyWindow();
}

LRESULT CTitleFormatSandboxDialog::OnScriptModified(LPNMHDR pnmh)
{
	//console::formatter() << "OnScriptModified";
	if (m_script_update_pending)
	{
		KillTimer(ID_SCRIPT_UPDATE_TIMER);
	}
	m_script_update_pending = true;
	SetTimer(ID_SCRIPT_UPDATE_TIMER, 250);
	return 0;
}

LRESULT CTitleFormatSandboxDialog::OnScriptUpdateUI(LPNMHDR pnmh)
{
	//console::formatter() << "OnScriptUpdateUI";
	if (!m_script_update_pending && GetFocus() == m_editor)
	{
		int selStart = m_editor.GetSelectionStart();
		int selEnd = m_editor.GetSelectionEnd();

		UpdateFragment(selStart, selEnd);
	}
	return 0;
}

LRESULT CTitleFormatSandboxDialog::OnTreeSelChanged(LPNMHDR pnmh)
{
	if (!m_script_update_pending)
	{
		LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmh;
		if (pnmtv->itemNew.hItem != 0 && pnmtv->itemNew.lParam != 0)
		{
			ast::node *n = (ast::node *)pnmtv->itemNew.lParam;
			UpdateFragment(n->get_start(), n->get_end());
		}
	}
	return 0;
}

void CTitleFormatSandboxDialog::OnSelectAll(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CWindow wnd = GetFocus();
	if (wnd == m_editor ||
		wnd == m_sciLexerFragment ||
		wnd == m_editStringValue ||
		wnd == m_editBoolValue)
	{
		CEdit edit = wnd;
		edit.SetSelAll();
	}
}

void CTitleFormatSandboxDialog::UpdateScript()
{
	KillTimer(ID_SCRIPT_UPDATE_TIMER);
	m_script_update_pending = false;

	m_treeScript.DeleteAllItems();

	ClearFragment();
	ClearInactiveCodeIndicator();

	CStringA format;
	m_editor.GetText(format);
	cfg_format = format;

	pfc::string_formatter errors;

	try
	{
		pfc::hires_timer parse_timer;
		parse_timer.start();
		m_debugger.parse(format);
		double parse_time = parse_timer.query();

		int error_count = m_debugger.get_parser_errors(errors);

		//console::formatter() << "[foo_tfsandbox] Script parsed in "<<pfc::format_float(parse_time, 6)<<"s";

		if (error_count == 0)
		{
			UpdateTrace();
		}
		else
		{
			uSetWindowText(m_editStringValue, errors);
		}
	}
	catch (std::exception const & exc)
	{
		console::formatter() << "Exception in UpdateScript(): " << exc;
	}
}

namespace ast
{
	class visitor_build_syntax_tree : private visitor
	{
	public:
		visitor_build_syntax_tree(titleformat_debugger &_debugger) : debugger(_debugger) {}
		void run(CTreeViewCtrl _treeView, block_expression *root)
		{
			treeView = _treeView;
			parent = TVI_ROOT;
			param_index = ~0;
			root->accept(this);
		}

	private:
		CTreeViewCtrl treeView;
		HTREEITEM parent;
		t_size param_index;

		titleformat_debugger &debugger;

		typedef pfc::vartoggle_t<HTREEITEM> builder_helper;

		HTREEITEM InsertItem(const char *text, int image, node *n)
		{
			return treeView.InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM, U2T(text), image, image, 0, 0, (LPARAM)n, parent, TVI_LAST);
		}

		void visit(block_expression *n)
		{
			builder_helper helper(parent, parent);
			bool expand = false;
			if (param_index != ~0 && n->get_child_count() != 1)
			{
				parent = InsertItem("Block", n->kind(), n);
				expand = true;
			}

			const t_size count = n->get_child_count();
			for (t_size index = 0; index < count; ++index)
			{
				n->get_child(index)->accept(this);
			}

			if (expand) treeView.Expand(parent);
		}

		void visit(comment *n) {}

		void visit(string_constant *n)
		{
			InsertItem(n->get_text().get_ptr(), n->kind(), n);
		}

		void visit(field_reference *n)
		{
			InsertItem(n->get_field_name().get_ptr(), n->kind(), n);
		}

		void visit(condition_expression *n)
		{
			builder_helper helper(parent, parent);
			parent = InsertItem("[ ]", n->kind(), n);

			param_index = ~0;
			n->get_param()->accept(this);

			treeView.Expand(parent);
		}

		void visit(call_expression *n)
		{
			pfc::string name = n->get_function_name();
			t_size count = n->get_param_count();
			if (debugger.is_function_unknown(n))
			{
				pfc::string_formatter label;
				label << name.get_ptr() << " [unknown function]";

				InsertItem(label.get_ptr(), n->kind(), n);
			}
			else
			{
				builder_helper helper(parent, parent);

				parent = InsertItem(n->get_function_name().get_ptr(), n->kind(), n);

				t_size count = n->get_param_count();
				for (t_size index = 0; index < count; ++index)
				{
					param_index = index;
					n->get_param(index)->accept(this);
				}

				treeView.Expand(parent);
			}
		}
	};
} // namespace ast

void CTitleFormatSandboxDialog::UpdateScriptSyntaxTree()
{
	ast::visitor_build_syntax_tree(m_debugger).run(m_treeScript, m_debugger.get_root());
}

void CTitleFormatSandboxDialog::UpdateTrace()
{
	if (m_script_update_pending) return;

	m_selfrag = ast::fragment();

	metadb_handle_ptr track;
	metadb::g_get_random_handle(track);

	m_debugger.trace(track);
	UpdateInactiveCodeIndicator();
	UpdateScriptSyntaxTree();

	UpdateFragment(m_editor.GetSelectionStart(), m_editor.GetSelectionEnd());
}

static void g_walk_indicator(CSciLexerCtrl sciLexer, titleformat_debugger &dbg, ast::node *n)
{
	if (dbg.test_value(n))
	{
		t_size count = n->get_child_count();
		switch (n->kind())
		{
		case ast::node::kind_block:
			for (t_size index = 0; index < count; ++index)
			{
				g_walk_indicator(sciLexer, dbg, n->get_child(index));
			}
			break;

		case ast::node::kind_condition:
			g_walk_indicator(sciLexer, dbg, n->get_child(1));
			break;

		case ast::node::kind_call:
			ast::call_expression *call;
			call = static_cast<ast::call_expression *>(n);
			for (t_size index = 0; index < call->get_param_count(); ++index)
			{
				g_walk_indicator(sciLexer, dbg, call->get_param(index));
			}
			break;
		}
	}
	else if (n->kind() != ast::node::kind_comment)
	{
		ast::position pos = n->get_position();
		if (pos.start != -1)
		{
			sciLexer.IndicatorFillRange(pos.start, pos.end - pos.start);
		}
	}
}

void CTitleFormatSandboxDialog::ClearInactiveCodeIndicator()
{
	m_editor.SetIndicatorCurrent(indicator_inactive_code);
	m_editor.SetIndicatorValue(1);
	m_editor.IndicatorClearRange(0, m_editor.GetTextLength());
}

void CTitleFormatSandboxDialog::UpdateInactiveCodeIndicator()
{
	m_editor.SetIndicatorCurrent(indicator_inactive_code);
	m_editor.SetIndicatorValue(1);
	m_editor.IndicatorClearRange(0, m_editor.GetTextLength());

	if (m_debugger.get_parser_errors() == 0)
	{
		ast::node *root = m_debugger.get_root();
		g_walk_indicator(m_editor, m_debugger, root);
	}
}

void CTitleFormatSandboxDialog::ClearFragment()
{
	m_selfrag = ast::fragment();

	m_editor.SetIndicatorCurrent(indicator_fragment);
	m_editor.SetIndicatorValue(1);
	m_editor.IndicatorClearRange(0, m_editor.GetTextLength());
}

void CTitleFormatSandboxDialog::UpdateFragment(int selStart, int selEnd)
{
	if (m_script_update_pending || m_updating_fragment) return;

	pfc::vartoggle_t<bool> blah(m_updating_fragment, true);

	pfc::string_formatter errors, value_string, value_bool;

	try
	{
		if (m_debugger.get_parser_errors() == 0)
		{
			ast::fragment selfrag;
			find_fragment(selfrag, selStart, selEnd);

			bool fragment_changed = m_selfrag != selfrag;

			if (!fragment_changed) return;

			m_selfrag = selfrag;

			bool flag = false;
			bool flag_set = false;
			for (t_size index = 0; index < selfrag.get_count(); ++index)
			{
				if (selfrag[index]->kind() != ast::node::kind_comment)
				{
					pfc::string_formatter temp_string;
					bool temp_bool = false;
					if (m_debugger.get_value(selfrag[index], temp_string, temp_bool))
					{
						flag_set = true;
						value_string << temp_string;
						if (temp_bool) flag = true;
					}
				}
			}
			if (flag_set) value_bool = flag ? "true" : "false";

			m_editor.SetIndicatorCurrent(indicator_fragment);
			m_editor.SetIndicatorValue(1);
			m_editor.IndicatorClearRange(0, m_editor.GetTextLength());
			for (t_size index = 0; index < selfrag.get_count(); ++index)
			{
				ast::position pos = selfrag[index]->get_position();
				if (pos.start != -1)
				{
					m_editor.IndicatorFillRange(pos.start, pos.end - pos.start);
				}
			}

			if (errors.length() > 0) errors << "\r\n";
			errors << value_string;
		}
	}
	catch (std::exception const & exc)
	{
		console::formatter() << "Exception in UpdateFragment(): " << exc;
	}

	m_sciLexerFragment.Call(SCI_SETREADONLY, false);
	if (m_selfrag.get_count() > 0)
	{
		TextRange tr;
		tr.chrg.cpMin = m_selfrag[0]->get_start();
		tr.chrg.cpMax = m_selfrag[m_selfrag.get_count() - 1]->get_end();
		pfc::array_t<char> buffer;
		buffer.set_size(tr.chrg.cpMax - tr.chrg.cpMin + 1);
		tr.lpstrText = buffer.get_ptr();
		m_editor.Call(SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
		m_sciLexerFragment.SetText(buffer.get_ptr());
	}
	else
	{
		m_sciLexerFragment.SetText("");
	}
	m_sciLexerFragment.Call(SCI_SETREADONLY, true);
	uSetWindowText(m_editStringValue, errors);
	uSetWindowText(m_editBoolValue, value_bool);
}

void CTitleFormatSandboxDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == ID_SCRIPT_UPDATE_TIMER)
	{
		UpdateScript();
	}
}

LRESULT CTitleFormatSandboxDialog::OnScriptDwellStart(LPNMHDR pnmh)
{
	SCNotification *scn = (SCNotification *)pnmh;

	if (!m_script_update_pending && m_debugger.get_parser_errors() == 0)
	{
		ast::fragment frag;
		if (find_fragment(frag, scn->position))
		{
			if (frag.get_count() == 1)
			{
				ast::position pos = frag[0]->get_position();
				pfc::string_formatter msg;
				switch (frag[0]->kind())
				{
				case ast::node::kind_comment:
					msg << "Comment";
					break;
				case ast::node::kind_string:
					msg << "String constant";
					break;
				case ast::node::kind_field:
					msg << "Field reference";
					break;
				case ast::node::kind_condition:
					msg << "Conditional section";
					break;
				case ast::node::kind_call:
					msg << "Function call";
					break;
				}

				pfc::string_formatter string_value;
				bool bool_value;
				if (m_debugger.get_value(frag[0], string_value, bool_value))
				{
					int tabSize = pfc::max_t(
						m_editor.TextWidth(STYLE_CALLTIP, "Text value: "),
						m_editor.TextWidth(STYLE_CALLTIP, "Boolean value: "));
					m_editor.CallTipUseStyle(tabSize);
					bool truncated = string_value.truncate_eol() || string_value.limit_length(250, "");
					msg << "\nText value:\t" << string_value;
					if (truncated) msg << "...";
					msg << "\nBoolean value:\t" << (bool_value ? "true" : "false");
				}
				else
				{
					msg << "\nNot evaluated";
				}
				m_editor.CallTipCancel();
				m_editor.CallTipShow(pos.start, msg);
			}
		}
	}

	return 0;
}

LRESULT CTitleFormatSandboxDialog::OnScriptDwellEnd(LPNMHDR pnmh)
{
	m_editor.CallTipCancel();
	return 0;
}

LRESULT CTitleFormatSandboxDialog::OnScriptCallTipClick(LPNMHDR pnmh)
{
	m_editor.CallTipCancel();
	return 0;
}
