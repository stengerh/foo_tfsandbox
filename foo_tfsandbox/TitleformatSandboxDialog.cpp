#include "stdafx.h"
#include <vector>

#include "Scintilla.h"
#include "SciLexer.h"
#include "atlscilexer.h"

#include "TitleformatSandboxDialog.h"
#include "titleformat_node_filter.h"
#include "titleformat_visitor_impl.h"

#ifndef U2T
#define U2T(Text) pfc::stringcvt::string_os_from_utf8(Text).get_ptr()
#endif

#define USE_EXPLORER_THEME 1

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

static COLORREF BlendColor(COLORREF color1, DWORD weight1, COLORREF color2, DWORD weight2)
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
			//pixels[base + 3] = a;
		}
	}

	sciLexer.RGBAImageSetWidth(16);
	sciLexer.RGBAImageSetHeight(16);
	sciLexer.RGBAImageSetScale(100);
	sciLexer.MarkerDefineRGBAImage(markerNumber, &pixels[0]);
}

CWindow CTitleFormatSandboxDialog::g_wndInstance;

bool CTitleFormatSandboxDialog::find_fragment(ast::fragment &out, int start, int end)
{
	return ast::find_fragment(out, m_debugger.get_root(), start, end, &ast::node_filter_impl_unknown_function(m_debugger));
}

CTitleFormatSandboxDialog::CTitleFormatSandboxDialog() : m_dlgPosTracker(cfg_window_position)
{
	m_privateCall = NULL;

	pfc::string8 install_dir = pfc::string_directory(core_api::get_my_full_path());

	m_sciLexerScope.LoadLibrary(TEXT("SciLexer.dll"));

	pfc::string8 lextitleformat_path = install_dir;
	lextitleformat_path += "\\LexTitleformat.dll";

	m_lexTitleformatScope.LoadLibrary(pfc::stringcvt::string_os_from_utf8(lextitleformat_path.get_ptr()));
}

CTitleFormatSandboxDialog::~CTitleFormatSandboxDialog()
{
	if (m_privateCall != NULL)
	{
		m_privateCall->Release();
		m_privateCall = NULL;
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
	}
	else
	{
		g_wndInstance.SetFocus();
	}
}

void CTitleFormatSandboxDialog::SetupTitleFormatStyles(CSciLexerCtrl sciLexer)
{
	m_editor.SetMarginTypeN(0, SC_MARGIN_NUMBER);
	m_editor.SetMarginWidthN(0, m_editor.TextWidth(STYLE_LINENUMBER, "_99"));

	sciLexer.SetMarginTypeN(1, SC_MARGIN_BACK);
	sciLexer.SetMarginMaskN(1, SC_MASK_FOLDERS);
	sciLexer.SetMarginWidthN(1, 16);
	sciLexer.SetMarginSensitiveN(1, true);
	
	sciLexer.SetFoldMarginColour(true, RGB(255, 255, 255));

	int markerNumbers[] = {
		SC_MARKNUM_FOLDEROPEN,
		SC_MARKNUM_FOLDER,
		SC_MARKNUM_FOLDERSUB,
		SC_MARKNUM_FOLDERTAIL,
		SC_MARKNUM_FOLDEREND,
		SC_MARKNUM_FOLDEROPENMID,
		SC_MARKNUM_FOLDERMIDTAIL
	};

	for (int index = 0; index < (sizeof(markerNumbers) / sizeof(markerNumbers[0])); ++index)
	{
		int markerNumber = markerNumbers[index];
		sciLexer.MarkerSetFore(markerNumber, RGB(255, 255, 255));
		sciLexer.MarkerSetBack(markerNumber, RGB(128, 128, 128));
		sciLexer.MarkerSetBackSelected(markerNumber, RGB(0, 0, 255));
	}

	sciLexer.MarkerDefine(SC_MARKNUM_FOLDEROPEN,    SC_MARK_BOXMINUS);
	sciLexer.MarkerDefine(SC_MARKNUM_FOLDER,        SC_MARK_BOXPLUS);
	sciLexer.MarkerDefine(SC_MARKNUM_FOLDERSUB,     SC_MARK_VLINE);
	sciLexer.MarkerDefine(SC_MARKNUM_FOLDERTAIL,    SC_MARK_LCORNER);
	sciLexer.MarkerDefine(SC_MARKNUM_FOLDEREND,     SC_MARK_BOXPLUSCONNECTED);
	sciLexer.MarkerDefine(SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	sciLexer.MarkerDefine(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

	sciLexer.SetAutomaticFold(SC_AUTOMATICFOLD_CLICK);

	sciLexer.SetWrapMode(SC_WRAP_WORD);
	sciLexer.SetWrapVisualFlags(SC_WRAPVISUALFLAG_END);

	sciLexer.StyleSetFont(STYLE_DEFAULT, "Consolas");
	//sciLexer.StyleSetSizeFractional(STYLE_DEFAULT, 800);
	sciLexer.StyleSetFore(STYLE_DEFAULT, RGB(0, 0, 0));
	sciLexer.StyleSetBack(STYLE_DEFAULT, RGB(255, 255, 255));
	sciLexer.StyleClearAll();

	sciLexer.SetSelFore(true, RGB(255, 255, 255));
	sciLexer.SetSelBack(true, RGB(51, 153, 255));

	sciLexer.CallTipUseStyle(50);

	sciLexer.StyleSetFont(STYLE_CALLTIP, "Verdana");
	sciLexer.StyleSetFore(STYLE_CALLTIP, ::GetSysColor(COLOR_INFOTEXT));
	sciLexer.StyleSetBack(STYLE_CALLTIP, ::GetSysColor(COLOR_INFOBK));

	const COLORREF background = RGB(255, 255, 255);

	m_editor.LoadLexerLibrary("LexTitleformat.dll");

	sciLexer.SetLexerLanguage("titleformat");
	ATL::CStringA lexerLanguage;
	if ((sciLexer.GetLexerLanguage(lexerLanguage) > 0) && (lexerLanguage == "titleformat"))
	{
		sciLexer.SetProperty("fold", "1");

		m_privateCall = (ILexerTitleformatPrivateCall *) sciLexer.PrivateLexerCall(SPC_TITLEFORMAT_GETINTERFACE, NULL);

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
	}

	// inactive code
	//sciLexer.IndicSetStyle(indicator_inactive_code, INDIC_DIAGONAL);
	//sciLexer.IndicSetFore(indicator_inactive_code, RGB(64, 64, 64));
	sciLexer.IndicSetStyle(indicator_inactive_code, INDIC_STRAIGHTBOX);
	sciLexer.IndicSetFore(indicator_inactive_code, RGB(0, 0, 0));
	sciLexer.IndicSetUnder(indicator_inactive_code, true);
	sciLexer.Call(SCI_INDICSETALPHA, indicator_inactive_code, 20);
	sciLexer.Call(SCI_INDICSETOUTLINEALPHA, indicator_inactive_code, 20);

	// selected fragment
	sciLexer.IndicSetStyle(indicator_fragment, INDIC_STRAIGHTBOX);
	sciLexer.IndicSetFore(indicator_fragment, RGB(64, 128, 255));
	sciLexer.IndicSetUnder(indicator_fragment, true);

	// errors
	sciLexer.IndicSetStyle(indicator_error, INDIC_SQUIGGLE);
	sciLexer.IndicSetFore(indicator_error, RGB(255, 0, 0));

}

void CTitleFormatSandboxDialog::SetupPreviewStyles(CSciLexerCtrl sciLexer)
{
	sciLexer.SetWrapMode(SC_WRAP_WORD);
	sciLexer.SetWrapVisualFlags(SC_WRAPVISUALFLAG_END);

	sciLexer.SetMarginTypeN(0, SC_MARGIN_NUMBER);
	sciLexer.SetMarginWidthN(0, sciLexer.TextWidth(STYLE_LINENUMBER, "_99"));

	sciLexer.SetMarginTypeN(1, SC_MARGIN_BACK);
	sciLexer.SetMarginWidthN(1, 16);

	LoadMarkerIcon(sciLexer, 0, MAKEINTRESOURCE(IDI_FUGUE_CROSS));
	LoadMarkerIcon(sciLexer, 1, MAKEINTRESOURCE(IDI_FUGUE_TICK));
	LoadMarkerIcon(sciLexer, 2, MAKEINTRESOURCE(IDI_FUGUE_EXCLAMATION));

	sciLexer.StyleSetFore(STYLE_DEFAULT, RGB(0, 0, 0));
	sciLexer.StyleSetBack(STYLE_DEFAULT, RGB(255, 255, 255));

	sciLexer.SetSelFore(true, RGB(255, 255, 255));
	sciLexer.SetSelBack(true, RGB(51, 153, 255));
}

BOOL CTitleFormatSandboxDialog::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_editor.Attach(GetDlgItem(IDC_SCRIPT));
	m_preview.Attach(GetDlgItem(IDC_VALUE));
	m_treeScript.Attach(GetDlgItem(IDC_TREE));

	m_editor.SetCodePage(SC_CP_UTF8);
	m_editor.SetModEventMask(SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);
	m_editor.SetMouseDwellTime(500);

	SetIcon(static_api_ptr_t<ui_control>()->get_main_icon());

	// Set up styles
	SetupTitleFormatStyles(m_editor);

	SetupPreviewStyles(m_preview);

	m_preview.SetReadOnly(true);

#if defined USE_EXPLORER_THEME
	SetWindowTheme(m_treeScript, L"Explorer", NULL);
#endif

	CImageList imageList;

#if defined(USE_EXPLORER_THEME)
	imageList.CreateFromImage(IDB_SYMBOLS32, 16, 2, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
#else
	imageList.CreateFromImage(IDB_SYMBOLS, 16, 2, RGB(255, 0, 255), IMAGE_BITMAP);
#endif
	imageList.SetOverlayImage(6, 1);
	
	m_treeScript.SetImageList(imageList);

	m_script_update_pending = true;
	//m_updating_fragment = false;


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

LRESULT CTitleFormatSandboxDialog::OnTreeCustomDraw(LPNMHDR pnmh)
{
	LPNMTVCUSTOMDRAW pnmcd = (LPNMTVCUSTOMDRAW) pnmh;
	LRESULT lResult = CDRF_DODEFAULT;

	switch (pnmcd->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		lResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		if (!m_script_update_pending)
		{
			bool replaceTextColor;
#ifdef USE_EXPLORER_THEME
			replaceTextColor = true;
#else
			replaceTextColor = (pnmcd->nmcd.uItemState & (CDIS_SELECTED)) == 0;
#endif
			if (pnmcd->nmcd.lItemlParam != 0 && replaceTextColor)
			{
				ast::node *n = (ast::node *)pnmcd->nmcd.lItemlParam;
				bool active = m_debugger.test_value(n);

				COLORREF colorBackground = RGB(255, 255, 255);
				
				switch (n->kind())
				{
				case ast::node::kind_block:
					pnmcd->clrText = active ? RGB(0, 0, 0) : BlendColor(RGB(0, 0, 0), 1, colorBackground, 1);
					break;
				case ast::node::kind_call:
					pnmcd->clrText = active ? RGB(192, 0, 192) : BlendColor(RGB(192, 0, 192), 1, colorBackground, 1);
					break;
				case ast::node::kind_comment:
					pnmcd->clrText = RGB(0, 192, 0);
					break;
				case ast::node::kind_condition:
					pnmcd->clrText = active ? RGB(0, 0, 0) : BlendColor(RGB(0, 0, 0), 1, colorBackground, 1);
					break;
				case ast::node::kind_field:
					pnmcd->clrText = active ? RGB(0, 0, 192) : BlendColor(RGB(0, 0, 192), 1, colorBackground, 1);
					break;
				case ast::node::kind_string:
					pnmcd->clrText = active ? RGB(0, 0, 0) : BlendColor(RGB(0, 0, 0), 1, colorBackground, 1);
					break;
				}
			}
		}
		break;
	}

	return lResult;
}

void CTitleFormatSandboxDialog::OnSelectAll(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CWindow wnd = GetFocus();
	if (wnd == m_editor ||
		wnd == m_preview)
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
			m_preview.MarkerDeleteAll(-1);
			m_preview.SetReadOnly(false);
			m_preview.SetText(errors);
			m_preview.SetReadOnly(true);
			m_preview.MarkerAdd(0, 2);
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
			param_index = 0;
			root->accept(this);
		}

	private:
		CTreeViewCtrl treeView;
		HTREEITEM parent;
		t_size param_index;

		titleformat_debugger &debugger;

		typedef pfc::vartoggle_t<HTREEITEM> builder_helper;

		HTREEITEM InsertItem(const char *text, int image, node *n, bool warning = false)
		{
			return treeView.InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_STATE,
				U2T(text), image, image,
				warning ? INDEXTOOVERLAYMASK(1) : 0,
				warning ? LVIS_OVERLAYMASK : 0,
				(LPARAM)n, parent, TVI_LAST);
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
			builder_helper helper(parent, parent);

			parent = InsertItem(n->get_function_name().get_ptr(), n->kind(), n, debugger.is_function_unknown(n));

			t_size count = n->get_param_count();
			for (t_size index = 0; index < count; ++index)
			{
				param_index = index;
				n->get_param(index)->accept(this);
			}

			treeView.Expand(parent);
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

void CTitleFormatSandboxDialog::ClearInactiveCodeIndicator()
{
	if (m_privateCall != NULL)
	{
		m_privateCall->ClearInactiveRanges();
	}
	else
	{
		m_editor.SetIndicatorCurrent(indicator_inactive_code);
		m_editor.SetIndicatorValue(1);
		m_editor.IndicatorClearRange(0, m_editor.GetTextLength());
	}
}

void CTitleFormatSandboxDialog::UpdateInactiveCodeIndicator()
{
	inactive_range_walker walker(m_debugger);
	if (m_privateCall != NULL)
	{
		if (!walker.inactiveRanges.empty())
		{
			m_privateCall->SetInactiveRanges(walker.inactiveRanges.size(), &walker.inactiveRanges[0]);
			m_editor.Colourise(0, m_editor.GetLength());
		}
	}
	else
	{
		m_editor.SetIndicatorCurrent(indicator_inactive_code);
		m_editor.SetIndicatorValue(1);
		m_editor.IndicatorClearRange(0, m_editor.GetTextLength());

		if (m_debugger.get_parser_errors() == 0)
		{
			inactive_range_walker walker(m_debugger);
			size_t count = walker.inactiveRanges.size();
			for (size_t index = 0; index < count; ++index)
			{
				int position = walker.inactiveRanges[index].cpMin;
				int fillLength = walker.inactiveRanges[index].cpMax - walker.inactiveRanges[index].cpMin;
				m_editor.IndicatorFillRange(position, fillLength);
			}
		}
	}
}

void CTitleFormatSandboxDialog::ClearFragment()
{
	m_selfrag = ast::fragment();

	if (m_privateCall != NULL)
	{
		m_privateCall->ClearInactiveRanges();
		m_editor.Colourise(0, m_editor.GetLength());
	}
	else
	{
		m_editor.SetIndicatorCurrent(indicator_fragment);
		m_editor.SetIndicatorValue(1);
		m_editor.IndicatorClearRange(0, m_editor.GetTextLength());
	}
}

void CTitleFormatSandboxDialog::UpdateFragment(int selStart, int selEnd)
{
	if (m_script_update_pending/* || m_updating_fragment*/) return;

	//pfc::vartoggle_t<bool> blah(m_updating_fragment, true);

	try
	{
		if (m_debugger.get_parser_errors() == 0)
		{
			ast::fragment selfrag;
			find_fragment(selfrag, selStart, selEnd);

			bool fragment_changed = m_selfrag != selfrag;

			if (!fragment_changed) return;

			m_selfrag = selfrag;

			bool fragment_value_defined = false;
			pfc::string_formatter fragment_string_value;
			bool fragment_bool_value = false;

			for (t_size index = 0; index < selfrag.get_count(); ++index)
			{
				if (selfrag[index]->kind() != ast::node::kind_comment)
				{
					pfc::string_formatter node_string_value;
					bool node_bool_value = false;

					if (m_debugger.get_value(selfrag[index], node_string_value, node_bool_value))
					{
						fragment_value_defined = true;
						fragment_bool_value |= node_bool_value;
						fragment_string_value << node_string_value;
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

			m_preview.MarkerDeleteAll(-1);
			m_preview.SetReadOnly(false);
			m_preview.SetText(fragment_string_value);
			m_preview.SetReadOnly(true);

			if (fragment_value_defined)
			{
				m_preview.MarkerAdd(0, fragment_bool_value ? 1 : 0);
			}
		}
	}
	catch (std::exception const & exc)
	{
		console::formatter() << "Exception in UpdateFragment(): " << exc;
	}
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
