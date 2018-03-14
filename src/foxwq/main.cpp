#include "spy.h"
#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include "resource.h"
#include <map>


#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")

#pragma comment    (lib, "comctl32.lib")   

#define WINDOW_FINDER_APP_MUTEX_NAME	_T("WINDOWFINDERMUTEX")
#define BULLSEYE_CENTER_X_OFFSET		15
#define BULLSEYE_CENTER_Y_OFFSET		18



BOOL InitializeApplication
(
	HINSTANCE hThisInst,
	HINSTANCE hPrevInst,
	LPTSTR lpszArgs,
	int nWinMode
	);
BOOL UninitializeApplication();


HINSTANCE	g_hInst = NULL;
HANDLE		g_hApplicationMutex = NULL;
DWORD		g_dwLastError = 0;

TCHAR		g_szLocalPath[256];


Dialog* Dialog::current = nullptr;


Dialog* Dialog::create(HINSTANCE hInst) {
	auto hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_MAIN_WINDOW), 0, Proc, 0);
	//current is assigned in INIT_DIALOG
	return current;
}


void Dialog::show(int nCmdShow) {
	ShowWindow(hWnd_, nCmdShow);
}

int Dialog::messageLoop() {

	BOOL ret;
	MSG msg;

	while ((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
		if (ret == -1) /* error found */
			return -1;

		if (!IsDialogMessage(hWnd_, &msg)) {
			TranslateMessage(&msg); /* translate virtual-key messages */
			DispatchMessage(&msg); /* send it to dialog procedure */
		}
	}
	return 0;
}

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE h0, LPTSTR lpCmdLine, int nCmdShow) {

	g_hInst = hInst;

	InitializeApplication(hInst, h0, lpCmdLine, nCmdShow);
	InitCommonControls();

	auto* dlg = Dialog::create(hInst);
	dlg->show(nCmdShow);
	if (dlg->messageLoop() < 0)
		return -1;

	UninitializeApplication();
	delete dlg;
	return 0;
}





#define IDT_TIMER1 100
#define IDT_TIMER_LABEL 101

Dialog::Dialog(HWND wnd)
: hWnd_(wnd)
, wndLabel_(spy)
{
	bStartSearchWindow = FALSE;
}

Dialog::~Dialog()
{
	current = nullptr;
}

INT_PTR CALLBACK Dialog::Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (!current) {
		if (uMsg == WM_INITDIALOG) {
			current = new Dialog(hDlg);
			current->onInit();
		}

		return FALSE;
	}

	return current->Proc(uMsg, wParam, lParam);
}


INT_PTR CALLBACK Dialog::Proc(UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_CLOSE:
		onClose();
		return TRUE;
	case WM_DESTROY:
		onDestroy();
		return TRUE;
	case WM_TIMER:
		switch (wParam) {
		case IDT_TIMER1:
			onDetectInterval();
			return TRUE;
		}
		return FALSE;
	case WM_COMMAND: {
		WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier 

		if (wID == IDC_BTN_PONDER) {
			//spy.stop_think();
			return TRUE;
		}

		if (wID == IDC_BTN_HINT) {
			spy.hint();
			return TRUE;
		}

		if (wID == IDC_CHECK_AUTO) {
			bool hint = IsDlgButtonChecked(hWnd_, wID);
			auto_play_ = hint;
			return TRUE;
		}
	}

	}
	return 0;
}

void Dialog::onInit() {

	auto_play_ = false;
	connected_ = false;

	spy.initResource();

	SetTimer(hWnd_,             // handle to main window 
		IDT_TIMER1,            // timer identifier 
		20,                 // 10-second interval 
		(TIMERPROC)NULL);     // no timer callback 

	wndLabel_.create(hWnd_);

	szMoves_[0] = 0;
	spy.onGtpIn = [&](const std::string& line) {
		//wndLabel_.hide();
		//strcat(szMoves_, line.c_str());
		//strcat(szMoves_, _T("\r\n"));
		//SetDlgItemText(hWnd_, IDC_EDIT_STATUS, szMoves_);
		auto str = line + "\r\n";

		auto hEdit = GetDlgItem(hWnd_, IDC_EDIT_STATUS);
		SendMessageA(hEdit, EM_SETSEL, 0, -1); //Select all
		SendMessageA(hEdit, EM_SETSEL, -1, -1);//Unselect and stay at the end pos
		SendMessageA(hEdit, EM_REPLACESEL, 0, (LPARAM)(str.c_str())); //append text to current pos and 
	};
	
	spy.onGtpOut = [&](const std::string& line) {
		//wndLabel_.hide();
		//strcat(szMoves_, line.c_str());
		//strcat(szMoves_, _T("\r\n"));
		//SetDlgItemText(hWnd_, IDC_EDIT_STATUS, szMoves_);
		auto str = line + "\r\n";

		auto hEdit = GetDlgItem(hWnd_, IDC_EDIT_STATUS);
		SendMessageA(hEdit, EM_SETSEL, 0, -1); //Select all
		SendMessageA(hEdit, EM_SETSEL, -1, -1);//Unselect and stay at the end pos
		SendMessageA(hEdit, EM_REPLACESEL, 0, (LPARAM)(str.c_str())); //append text to current pos and 
	};

	spy.onThinkBegin = [&]() {
		EnableWindow(GetDlgItem(hWnd_, IDC_BTN_HINT), FALSE);
		EnableWindow(GetDlgItem(hWnd_, IDC_BTN_PONDER), TRUE);
	};

	spy.onThinkEnd = [&]() {
		EnableWindow(GetDlgItem(hWnd_, IDC_BTN_HINT), TRUE);
		EnableWindow(GetDlgItem(hWnd_, IDC_BTN_PONDER), FALSE);
	};

	spy.placeStone = [&](int x, int y) {

		bool show = false;
		if (wndLabel_.isVisible()) {
			wndLabel_.hide();
			show = true;
			Sleep(10);
		}

		spy.placeAt(x, y);

		if (show)
			wndLabel_.show();
	};

	spy.onMoveCommit = [&](bool black_player, int move) {

		wndLabel_.hide();
/*
		TCHAR wbuf[50];
		wsprintf(wbuf, _T("%d: %s (%d,%d)"), steps, player == 1 ? _T("B") : _T("W"), x, y);
		if (steps == 1)
			lstrcpy(szMoves_, _T("[                                         ]\r\n"));

		lstrcat(szMoves_, wbuf);
		lstrcat(szMoves_, _T("\r\n"));
		memcpy(&szMoves_[1], wbuf, lstrlen(wbuf)*sizeof(TCHAR));
		SetDlgItemText(hWnd_, IDC_EDIT_STATUS, szMoves_);*/
	};

	spy.onSizeChanged = [&]() {
		wndLabel_.update();
	};

	spy.onThinkMove = [&](bool black_player, int move, const std::vector<std::pair<int,float>>& dist) {

		bool autoplay = auto_play_;

		int label = move;

		wndLabel_.setPos(label);
		if (autoplay) {

			spy.placeStone(move%19, move/19);
		}
		wndLabel_.show();
	};

	spy.onThinkPass = [&]() {
		MessageBox(NULL, _T("PASS"), TEXT("spy"), MB_ICONINFORMATION);
	};

	spy.onThinkResign = [&]() {
		MessageBox(NULL, _T("RESIGN"), TEXT("spy"), MB_ICONINFORMATION);
	};

	spy.onGtpReady = [&]() {
		updateStatus();
	};
}

void Dialog::updateStatus() {
	TCHAR msg[128];

	if (connected_) {
		lstrcpy(msg, _T("Located, "));
	}
	else {
		lstrcpy(msg, _T("Not Found, "));
	}

	SetDlgItemText(hWnd_, IDC_STATIC_INFO, msg);
}

void Dialog::onDetectInterval() {

	bool r = spy.routineCheck();
	
	if (r != connected_) {
		connected_ = r;
		updateStatus();
	}
}

void Dialog::onClose() {
	//DestroyWindow(hWnd_);
	if (MessageBox(hWnd_, TEXT("Exit?"), TEXT("Exit"),
		MB_ICONQUESTION | MB_YESNO) == IDYES)
	{
		spy.exit();
		DestroyWindow(hWnd_);
	}
}

void Dialog::onDestroy() {
	PostQuitMessage(0);
}



BOOL InitializeApplication
(
	HINSTANCE hThisInst,
	HINSTANCE hPrevInst,
	LPTSTR lpszArgs,
	int nWinMode
	)
{
	BOOL bRetTemp = FALSE;
	long lRetTemp = 0;
	BOOL bRet = FALSE;
	TCHAR* pos;

	// Create the application mutex so that no two instances of this app can run.
	g_hApplicationMutex = CreateMutex
		(
			(LPSECURITY_ATTRIBUTES)NULL, // pointer to security attributes 
			(BOOL)TRUE, // flag for initial ownership 
			(LPCTSTR)WINDOW_FINDER_APP_MUTEX_NAME // pointer to mutex-object name 
			);

	g_dwLastError = GetLastError();

	// If cannot create Mutex, exit.
	if (g_hApplicationMutex == NULL)
	{
		bRet = FALSE;
		goto InitializeApplication_0;
	}

	// If Mutex already existed, exit.
	if (g_dwLastError == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(g_hApplicationMutex);
		g_hApplicationMutex = NULL;
		bRet = FALSE;
		goto InitializeApplication_0;
	}

	GetModuleFileName(hThisInst, g_szLocalPath, sizeof(g_szLocalPath) / sizeof(TCHAR) - 1);
	pos = _tcsrchr(g_szLocalPath, _T('\\'));
	*pos = 0;

	// All went well, return a TRUE.
	bRet = TRUE;

InitializeApplication_0:

	return bRet;
}


BOOL UninitializeApplication()
{
	BOOL bRet = TRUE;

	if (g_hApplicationMutex)
	{
		ReleaseMutex(g_hApplicationMutex);
		CloseHandle(g_hApplicationMutex);
		g_hApplicationMutex = NULL;
	}

	return bRet;
}


PredictWnd* PredictWnd::current = nullptr;

void PredictWnd::create(HWND hParent) {
	current = this;
	hWnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hParent, Proc);
}

bool PredictWnd::isVisible() {
	return TRUE == IsWindowVisible(hWnd);
}

INT_PTR CALLBACK PredictWnd::Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (uMsg == WM_INITDIALOG) {
		current->hWnd = hDlg;
		current->onInit();
		return FALSE;
	}

	if (!current)
		return FALSE;

	return current->Proc(uMsg, wParam, lParam);
}

INT_PTR CALLBACK PredictWnd::Proc(UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_ERASEBKGND:
	{
		HBRUSH brush;
		RECT rect;
		brush = CreateSolidBrush(RGB(255, 0, 0));
		SelectObject((HDC)wParam, brush);
		GetClientRect(hWnd, &rect);
		Rectangle((HDC)wParam, rect.left, rect.top, rect.right, rect.bottom);
		return TRUE;
	}

	case WM_MOUSEMOVE: {
		mouseOver = true;
		ShowWindow(hWnd, SW_HIDE);

		SetTimer(hWnd,             // handle to main window 
			IDT_TIMER1,            // timer identifier 
			500,                 // 10-second interval 
			(TIMERPROC)NULL);     // no timer callback 

		return TRUE;
	}
	case WM_TIMER: {
		switch (wParam) {
		case IDT_TIMER1:
			mouseOver = false;
			if (!hiding_)
				ShowWindow(hWnd, SW_SHOW);
			return TRUE;
		}
		return TRUE;
	}
	}
	return FALSE;
}

void PredictWnd::onInit() {
	HRGN hrgnShape = createRegion();
	SetWindowPos(hWnd, NULL, 0, 0, size, size, 0);
	SetWindowRgn(hWnd, hrgnShape, TRUE);
}

PredictWnd::PredictWnd(BoardSpy& spy)
: hWnd(NULL)
, spy_(spy)
, mouseOver(false){
	if (!loadRes())
		fatal(_T("cannot find label resource"));
}


bool PredictWnd::loadRes() {
	TCHAR path[256];

	lstrcpy(path, g_szLocalPath);
	lstrcat(path, _T("\\res\\label.bmp"));

	std::vector<BYTE> data;
	int width, height;
	if (!loadBmpData(path, data, width, height))
		return false;

	if (width != height)
		return false;

	size = width;

	label_mask_data.resize(width*height);
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++) {
			auto idx = i*width + j;
			auto target = i*width * 3 + j * 3;
			auto r = data[target];
			auto g = data[target + 1];
			auto b = data[target + 2];

			if (r != 0 || g != 0 || b != 255) {
				label_mask_data[idx] = 1;
			}
			else {
				label_mask_data[idx] = 0;
			}
		}
	}
	return true;
}


HRGN PredictWnd::createRegion() {

	// We start with the full rectangular region
	HRGN hrgnBitmap = ::CreateRectRgn(0, 0, size, size);

	BOOL bInTransparency = FALSE;  // Already inside a transparent part?
	int start_x = -1;			   // Start of transparent part

								   // For all rows of the bitmap ...
	for (int y = 0; y < size; y++)
	{
		// For all pixels of the current row ...
		// (To close any transparent region, we go one pixel beyond the
		// right boundary)
		for (int x = 0; x <= size; x++)
		{
			BOOL bTransparent = FALSE; // Current pixel transparent?

									   // Check for positive transparency within image boundaries
			if ((x < size) && (label_mask_data[y*size + x] == 0))
			{
				bTransparent = TRUE;
			}

			// Does transparency change?
			if (bInTransparency != bTransparent)
			{
				if (bTransparent)
				{
					// Transparency starts. Remember x position.
					bInTransparency = TRUE;
					start_x = x;
				}
				else
				{
					// Transparency ends (at least beyond image boundaries).
					// Create a region for the transparency, one pixel high,
					// beginning at start_x and ending at the current x, and
					// subtract that region from the current bitmap region.
					// The remainding region becomes the current bitmap region.
					HRGN hrgnTransp = ::CreateRectRgn(start_x, y, x, y + 1);
					::CombineRgn(hrgnBitmap, hrgnBitmap, hrgnTransp, RGN_DIFF);
					::DeleteObject(hrgnTransp);

					bInTransparency = FALSE;
				}
			}
		}
	}

	return hrgnBitmap;
}

void PredictWnd::setPos(int movePos) {
	move_ = movePos;
	auto x = movePos % 19, y = movePos / 19;
	auto pt = spy_.coord2Screen(x, y);
	SetWindowPos(hWnd, NULL, pt.x - size / 2, pt.y - size / 2, size, size, 0);
}

void PredictWnd::update() {
	setPos(move_);
}

void PredictWnd::show() {

	hiding_ = false;

	if (!mouseOver)
		ShowWindow(hWnd, SW_SHOW);
}

void PredictWnd::hide() {
	hiding_ = true;
	ShowWindow(hWnd, SW_HIDE);
}

