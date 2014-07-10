#include "stdafx.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "atlscilexer.h"
#include "TFDbgDialog.h"

#include <vector>

#ifndef U2T
#define U2T(Text) pfc::stringcvt::string_os_from_utf8(Text).get_ptr()
#endif


// {B2E1B41E-CF32-41b2-884A-AE12D258FE71}
static const GUID guid_cfg_format = { 0xb2e1b41e, 0xcf32, 0x41b2, { 0x88, 0x4a, 0xae, 0x12, 0xd2, 0x58, 0xfe, 0x71 } };
// {2D33BBC8-7D6A-4a5e-9998-6ED8274F52C6}
static const GUID guid_cfg_window_position = { 0x2d33bbc8, 0x7d6a, 0x4a5e, { 0x99, 0x98, 0x6e, 0xd8, 0x27, 0x4f, 0x52, 0xc6 } };

static cfgDialogPosition cfg_window_position(guid_cfg_window_position);

static cfg_string cfg_format(guid_cfg_format,
#ifdef _DEBUG
	"// This is a comment\r\n'test'test$$%%''\r\n$if(%track artist%,'['test'')\r\n// This is another comment\r\n[%comment%]"
#else
	"// Type your title formatting code here.\r\n"
	"// The tree view on the right side shows the structure of the code.\r\n"
	"// Put the cursor on an expression to see its value in the box below.\r\n"
	"// The symbol before the first line shows the truth value of the expression. A check mark means true and a cross means false.\r\n"
	"[%artist% - ]%title%"
#endif
);

namespace ATL
{
	int GetModuleFileName(HMODULE hModule, CString& strFileName)
	{
		TCHAR buffer[MAX_PATH] = {0};
		::SetLastError(ERROR_SUCCESS);
		DWORD nLength = ::GetModuleFileName(hModule, buffer, MAX_PATH);
		if (nLength == MAX_PATH && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			DWORD nBufferSize = 256;
			do {
				nBufferSize *= 2;
				LPTSTR pszFileName = strFileName.GetBufferSetLength(nBufferSize);
				nLength = ::GetModuleFileName(hModule, pszFileName, nBufferSize);
				strFileName.ReleaseBuffer(nLength);
			} while (nLength == nBufferSize && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER);
		}
		else
		{
			strFileName.SetString(buffer, nLength);
		}

		return strFileName.GetLength();
	}
}

CLibraryScope::CLibraryScope() : m_hDll(NULL)
{
}

CLibraryScope::CLibraryScope(LPCTSTR pszName) : m_hDll(NULL)
{
	LoadLibrary(pszName);
}

bool CLibraryScope::LoadLibrary(LPCTSTR pszName)
{
	ATLASSERT(m_hDll == NULL);

	m_hDll = ::LoadLibrary(pszName);

	return m_hDll != NULL;
}

CLibraryScope::~CLibraryScope()
{
	if (m_hDll != NULL)
	{
		::FreeLibrary(m_hDll);
		m_hDll = NULL;
	}
}

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

bool CTitleFormatSandboxDialog::find_fragment(ast::fragment &out, int start, int end)
{
	return ast::find_fragment(out, m_debugger.get_root(), start, end, &ast::node_filter_impl_unknown_function(m_debugger));
}

CWindow CTitleFormatSandboxDialog::g_wndInstance;

CTitleFormatSandboxDialog::CTitleFormatSandboxDialog() : m_dlgPosTracker(cfg_window_position)
{
	pfc::string8 install_dir = pfc::string_directory(core_api::get_my_full_path());

	pfc::string8 scilexer_path = install_dir;
	scilexer_path += "\\SciLexer.dll";

	//m_sciLexerScope.LoadLibrary(pfc::stringcvt::string_os_from_utf8(scilexer_path.get_ptr()));
	m_sciLexerScope.LoadLibrary(TEXT("SciLexer.dll"));

	pfc::string8 lextitleformat_path = install_dir;
	lextitleformat_path += "\\LexTitleformat.dll";

	m_lexTitleformatScope.LoadLibrary(pfc::stringcvt::string_os_from_utf8(lextitleformat_path.get_ptr()));
}

CTitleFormatSandboxDialog::~CTitleFormatSandboxDialog()
{
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
#if 0
		if (dlg->m_hSciLexerDll != NULL)
		{
#endif
			if (dlg->Create(core_api::get_main_window()) != NULL)
			{
				dlg->ShowWindow(SW_SHOW);
			}
			else
			{
				DWORD dwError = ::GetLastError();
				pfc::string8 message;
				console::formatter() << core_api::get_my_file_name() << ": Could not create window: "
					<< (uFormatSystemErrorMessage(message, dwError) ? message.get_ptr() : pfc::format_hex(dwError).get_ptr());
			}
#if 0
		}
		else
		{
			console::formatter() << core_api::get_my_file_name() << ": Could not load SciLexer.dll";
			delete dlg;
		}
#endif
	}
	else
	{
		g_wndInstance.SetFocus();
	}
}

COLORREF BlendColor(COLORREF color1, DWORD weight1, COLORREF color2, DWORD weight2)
{
	int r = (GetRValue(color1) * weight1 + GetRValue(color2) * weight2) / (weight1 + weight2);
	int g = (GetGValue(color1) * weight1 + GetGValue(color2) * weight2) / (weight1 + weight2);
	int b = (GetBValue(color1) * weight1 + GetBValue(color2) * weight2) / (weight1 + weight2);
	return RGB(r, g, b);
}

static void LoadMarkerIcon(CSciLexerCtrl sciLexer, int markerNumber, LPCTSTR name)
{
	CIcon icon;
	icon.LoadIcon(name, 16, 16);

	ICONINFO iconInfo;
	BOOL succeeded = icon.GetIconInfo(&iconInfo);

	CBitmapHandle imageColor = iconInfo.hbmColor;

	BITMAP bitmap;
	succeeded = imageColor.GetBitmap(&bitmap);

	DWORD dwCount = bitmap.bmWidthBytes * bitmap.bmHeight;

	std::vector<char> pixels;
	pixels.resize(dwCount);

	DWORD dwBytesCopied = imageColor.GetBitmapBits(dwCount, (LPVOID) &pixels[0]);

	for (int y = 0; y < 16; ++y)
	{
		for (int x = 0; x < 16; ++x)
		{
			int base = (y * 16 + x) * 4;
			char r = pixels[base + 0];
			char g = pixels[base + 1];
			char b = pixels[base + 2];
			//char a = pixels[base + 3];
			pixels[base + 2] = r;
			pixels[base + 1] = g;
			pixels[base + 0] = b;
			//pixels[base + 0] = a;
		}
	}

	sciLexer.RGBAImageSetWidth(16);
	sciLexer.RGBAImageSetHeight(16);
	sciLexer.RGBAImageSetScale(100);
	sciLexer.MarkerDefineRGBAImage(markerNumber, &pixels[0]);
#if 0
	BITMAPINFO bitmapInfo;
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = 16;
	bitmapInfo.bmiHeader.biHeight = 16;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biSizeImage = 0;
	bitmapInfo.bmiHeader.biXPelsPerMeter = 300;
	bitmapInfo.bmiHeader.biYPelsPerMeter = 300;
	bitmapInfo.bmiHeader.biClrUsed = 0;
	bitmapInfo.bmiHeader.biClrImportant = 0;

	char * pixels = 0;

	CBitmap imageColor;
	imageColor.CreateDIBSection(NULL, &bitmapInfo, DIB_RGB_COLORS, (void **)&pixels, NULL, 0);

	imageColor.GetDIBits();
	CMemoryDC dc;

	CBitmap image;
	image.CreateDIBSection();
	image.CreateCompatibleBitmap(dc, 16, 16);

	LPCSTR pixels;
	sciLexer.MarkerDefineRGBAImage(markerNumber, pixels);
#endif
}

void CTitleFormatSandboxDialog::SetupTitleFormatStyles(CSciLexerCtrl sciLexer)
{
	sciLexer.SetMarginWidthN(1, 0);

	sciLexer.SetWrapMode(SC_WRAP_WORD);
	sciLexer.SetWrapVisualFlags(SC_WRAPVISUALFLAG_END);

	sciLexer.StyleSetFont(STYLE_DEFAULT, "Courier New");
	sciLexer.StyleSetFore(STYLE_DEFAULT, RGB(0, 0, 0));
	sciLexer.StyleSetBack(STYLE_DEFAULT, RGB(255, 255, 255));
	sciLexer.StyleClearAll();

	sciLexer.SetSelFore(true, RGB(255, 255, 255));
	sciLexer.SetSelBack(true, RGB(51, 153, 255));

	const COLORREF background = RGB(255, 255, 255);

	// Comments
	sciLexer.StyleSetFore(1 /*SCE_TITLEFORMAT_COMMENTLINE*/, RGB(0, 128, 0));
	sciLexer.StyleSetFore(1 + 64 /*SCE_TITLEFORMAT_COMMENTLINE | inactive*/, BlendColor(RGB(0, 128, 0), 1, background, 1));

	// Operators
	sciLexer.StyleSetFore(2 /*SCE_TITLEFORMAT_OPERATOR*/, RGB(0, 0, 0));
	sciLexer.StyleSetBold(2 /*SCE_TITLEFORMAT_OPERATOR*/, true);
	sciLexer.StyleSetFore(2 + 64 /*SCE_TITLEFORMAT_OPERATOR | inactive*/, BlendColor(RGB(0, 0, 0), 1, background, 1));
	sciLexer.StyleSetBold(2 + 64 /*SCE_TITLEFORMAT_OPERATOR | inactive*/, true);

	// Fields
	sciLexer.StyleSetFore(3 /*SCE_TITLEFORMAT_FIELD*/, RGB(0, 0, 192));
	sciLexer.StyleSetFore(3 + 64 /*SCE_TITLEFORMAT_FIELD | inactive*/, BlendColor(RGB(0, 0, 192), 1, background, 1));

	// Strings (Single quoted string)
	sciLexer.StyleSetFore(4 /*SCE_TITLEFORMAT_STRING*/, RGB(0, 0, 0));
	sciLexer.StyleSetItalic(4 /*SCE_TITLEFORMAT_STRING*/, true);
	sciLexer.StyleSetFore(4 + 64 /*SCE_TITLEFORMAT_STRING | inactive*/, BlendColor(RGB(0, 0, 0), 1, background, 1));
	sciLexer.StyleSetItalic(4 + 64 /*SCE_TITLEFORMAT_STRING | inactive*/, true);

	// Text (Unquoted string)
	sciLexer.StyleSetFore(5 /*SCE_TITLEFORMAT_LITERALSTRING*/, RGB(0, 0, 0));
	sciLexer.StyleSetFore(5 + 64 /*SCE_TITLEFORMAT_LITERALSTRING | inactive*/, BlendColor(RGB(0, 0, 0), 1, background, 1));

	// Characters (%%, &&, '')
	sciLexer.StyleSetFore(6 /*SCE_TITLEFORMAT_SPECIALSTRING*/, RGB(0, 0, 0));
	sciLexer.StyleSetFore(6 + 64/*SCE_TITLEFORMAT_SPECIALSTRING | inactive*/, BlendColor(RGB(0, 0, 0), 1, background, 1));

	// Functions
	sciLexer.StyleSetFore(7 /*SCE_TITLEFORMAT_IDENTIFIER*/, RGB(192, 0, 192));
	sciLexer.StyleSetFore(7 + 64 /*SCE_TITLEFORMAT_IDENTIFIER | inactive*/, BlendColor(RGB(192, 0, 192), 1, background, 1));

	sciLexer.SetLexerLanguage("titleformat");
	ATL::CStringA lexerLanguage;
	if ((sciLexer.GetLexerLanguage(lexerLanguage) < 0) || (lexerLanguage != "titleformat"))
	{
		console::formatter() << core_api::get_my_file_name() << ": Could not select titleformat lexer";
	}
}

BOOL CTitleFormatSandboxDialog::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_editor.Attach(GetDlgItem(IDC_SCRIPT));
	m_value.Attach(GetDlgItem(IDC_VALUE));
	m_treeScript.Attach(GetDlgItem(IDC_TREE));

	m_editor.LoadLexerLibrary("LexTitleformat.dll");

	m_editor.SetCodePage(SC_CP_UTF8);
	m_editor.SetModEventMask(SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);
	m_editor.SetMouseDwellTime(500);


	m_editor.CallTipUseStyle(50);

	// Set up styles
	SetupTitleFormatStyles(m_editor);

	m_editor.SetMarginTypeN(0, SC_MARGIN_NUMBER);
	m_editor.SetMarginWidthN(0, m_editor.TextWidth(STYLE_LINENUMBER, "_99"));

	m_value.SetWrapMode(SC_WRAP_WORD);
	m_value.SetWrapVisualFlags(SC_WRAPVISUALFLAG_END);

	m_value.SetMarginTypeN(0, SC_MARGIN_NUMBER);
	m_value.SetMarginWidthN(0, m_value.TextWidth(STYLE_LINENUMBER, "_99"));

	m_value.SetMarginTypeN(1, SC_MARGIN_BACK);
	m_value.SetMarginWidthN(1, 16);

	m_value.SetReadOnly(true);

	m_editor.StyleSetFont(STYLE_CALLTIP, "Tahoma");
	m_editor.StyleSetFore(STYLE_CALLTIP, GetSysColor(COLOR_INFOTEXT));
	m_editor.StyleSetBack(STYLE_CALLTIP, GetSysColor(COLOR_INFOBK));

	// inactive code
	//m_editor.IndicSetStyle(indicator_inactive_code, INDIC_DIAGONAL);
	//m_editor.IndicSetFore(indicator_inactive_code, RGB(64, 64, 64));
	m_editor.IndicSetStyle(indicator_inactive_code, INDIC_STRAIGHTBOX);
	m_editor.IndicSetFore(indicator_inactive_code, RGB(0, 0, 0));
	m_editor.IndicSetUnder(indicator_inactive_code, true);
	m_editor.Call(SCI_INDICSETALPHA, indicator_inactive_code, 20);
	m_editor.Call(SCI_INDICSETOUTLINEALPHA, indicator_inactive_code, 20);

	// selected fragment
	m_editor.IndicSetStyle(indicator_fragment, INDIC_STRAIGHTBOX);
	m_editor.IndicSetFore(indicator_fragment, RGB(64, 128, 255));
	m_editor.IndicSetUnder(indicator_fragment, true);

	// errors
	m_editor.IndicSetStyle(indicator_error, INDIC_SQUIGGLE);
	m_editor.IndicSetFore(indicator_error, RGB(255, 0, 0));

	LoadMarkerIcon(m_value, 0, MAKEINTRESOURCE(IDI_FUGUE_CROSS));
	LoadMarkerIcon(m_value, 1, MAKEINTRESOURCE(IDI_FUGUE_TICK));
	LoadMarkerIcon(m_value, 2, MAKEINTRESOURCE(IDI_FUGUE_EXCLAMATION));

	CImageList imageList;
#ifdef USE_ICONS_WITH_ALPHA
	imageList.Create(16, 16, ILC_COLOR32, 6, 2);
	CBitmap image;
	image.Attach((HBITMAP) ::LoadImage(core_api::get_my_instance(), MAKEINTRESOURCE(IDB_SYMBOLS32), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	imageList.Add(image);
#else
	imageList.CreateFromImage(IDB_SYMBOLS, 16, 2, RGB(255, 0, 255), IMAGE_BITMAP);
#endif
	m_treeScript.SetImageList(imageList);

	m_script_update_pending = true;
	m_updating_fragment = false;


	m_editor.SetText(cfg_format);
	m_editor.SetEmptySelection(0);
	m_editor.EmptyUndoBuffer();
	m_editor.SetUndoCollection(true);

	m_accel.LoadAccelerators(IDD);

	static_api_ptr_t<message_loop>()->add_message_filter(this);

	g_wndInstance = *this;

	return TRUE;
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
	SCNotification * pnmsc = (SCNotification *) pnmh;
#ifdef _DEBUG
	{
		console::formatter fmt;
		fmt << "[foo_tfsandbox] OnScriptUpdateUI()";
		if ((pnmsc->updated & SC_UPDATE_CONTENT) != 0)
		{
			fmt << " SC_UPDATE_CONTENT";
		}
		if ((pnmsc->updated & SC_UPDATE_SELECTION) != 0)
		{
			fmt << " SC_UPDATE_SELECTION";
		}
		if ((pnmsc->updated & SC_UPDATE_H_SCROLL) != 0)
		{
			fmt << " SC_UPDATE_H_SCROLL";
		}
		if ((pnmsc->updated & SC_UPDATE_V_SCROLL) != 0)
		{
			fmt << " SC_UPDATE_V_SCROLL";
		}
	}
#endif

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
		wnd == m_value)
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

		console::formatter() << "[foo_tfsandbox] Script parsed in "<<pfc::format_float(parse_time, 6)<< " seconds";

		if (error_count == 0)
		{
			UpdateTrace();
		}
		else
		{
			m_value.SetReadOnly(false);
			m_value.SetText(errors);
			m_value.SetReadOnly(true);
			m_value.MarkerAdd(0, 2);
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
				parent = InsertItem("...", n->kind(), n);
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
			parent = InsertItem("[ ... ]", n->kind(), n);

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
	pfc::hires_timer build_timer;
	build_timer.start();

	ast::visitor_build_syntax_tree(m_debugger).run(m_treeScript, m_debugger.get_root());

	double build_time = build_timer.query();
	console::formatter() << "[foo_tfsandbox] Script structure built in " << pfc::format_float(build_time, 6) << " seconds";
}

void CTitleFormatSandboxDialog::UpdateTrace()
{
	if (m_script_update_pending) return;

	m_selfrag = ast::fragment();

	metadb_handle_ptr track;
	metadb::g_get_random_handle(track);

	pfc::hires_timer trace_timer;
	trace_timer.start();

	m_debugger.trace(track);

	double trace_time = trace_timer.query();

	console::formatter() << "[foo_tfsandbox] Script traced in " << pfc::format_float(trace_time, 6) << " seconds";

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

class inactive_range_walker
{
public:
	std::vector<int> inactiveRanges;

	inactive_range_walker(titleformat_debugger &p_dbg) : dbg(p_dbg)
	{
		walk(p_dbg.get_root());
		inactiveRanges.push_back(-1);
		inactiveRanges.push_back(-1);
	}

private:
	titleformat_debugger &dbg;

	void walk(ast::node *n)
	{
		if (dbg.test_value(n))
		{
			t_size count = n->get_child_count();
			switch (n->kind())
			{
			case ast::node::kind_block:
				for (t_size index = 0; index < count; ++index)
				{
					walk(n->get_child(index));
				}
				break;

			case ast::node::kind_condition:
				walk(n->get_child(1));
				break;

			case ast::node::kind_call:
				ast::call_expression *call;
				call = static_cast<ast::call_expression *>(n);
				for (t_size index = 0; index < call->get_param_count(); ++index)
				{
					walk(call->get_param(index));
				}
				break;
			}
		}
		else if (n->kind() != ast::node::kind_comment)
		{
			ast::position pos = n->get_position();
			if (pos.start != -1)
			{
				inactiveRanges.push_back(pos.start);
				inactiveRanges.push_back(pos.end - pos.start);
			}
		}
	}
};

void CTitleFormatSandboxDialog::ClearInactiveCodeIndicator()
{
#if 0
	m_editor.SetIndicatorCurrent(indicator_inactive_code);
	m_editor.SetIndicatorValue(1);
	m_editor.IndicatorClearRange(0, m_editor.GetTextLength());
#else
	int ranges[] = {
		-1, -1,
	};
	m_editor.PrivateLexerCall(1234, ranges);
#endif
}

void CTitleFormatSandboxDialog::UpdateInactiveCodeIndicator()
{
#if 0
	m_editor.SetIndicatorCurrent(indicator_inactive_code);
	m_editor.SetIndicatorValue(1);
	m_editor.IndicatorClearRange(0, m_editor.GetTextLength());

	if (m_debugger.get_parser_errors() == 0)
	{
		ast::node *root = m_debugger.get_root();
		g_walk_indicator(m_editor, m_debugger, root);
	}
#else
	inactive_range_walker walker(m_debugger);
	m_editor.PrivateLexerCall(1234, &walker.inactiveRanges[0]);
	m_editor.Colourise(0, m_editor.GetLength());
#endif
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

	pfc::string_formatter value_string;

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

			m_value.SetReadOnly(false);
			m_value.SetText(value_string);
			m_value.SetReadOnly(true);
			m_value.MarkerDeleteAll(0);
			m_value.MarkerDeleteAll(1);
			m_value.MarkerAdd(0, flag ? 1 : 0);
		}
	}
	catch (std::exception const & exc)
	{
		console::formatter() << "Exception in UpdateFragment(): " << exc;
	}
	//uSetWindowText(m_editStringValue, errors);
	//uSetWindowText(m_editBoolValue, value_bool);
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
