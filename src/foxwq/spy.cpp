
#include "spy.h"
#include <fstream>
#include <sstream>
#include <set>
#include "../tools.h"
#undef min


constexpr int min_hwnd_pixes = 500;
constexpr int THRESH_STONE_EXISTS_INTERVAL = 10;


static double square_diff(COLORREF c1, COLORREF c2) {
	auto r = std::abs(GetRValue(c1)-GetRValue(c2));
	auto g = std::abs(GetGValue(c1)-GetGValue(c2));
	auto b = std::abs(GetBValue(c1)-GetBValue(c2));
	return (255 - r*0.297 - g*0.593 - b*0.11) / 255;
}


struct stone_template {
	int value;
	LPCTSTR file;
};


static LPCTSTR mask_files[] = {
	_T("stone.bmp"),
	_T("d.bmp"),
};

BoardSpy::BoardSpy() 
: hTargetWnd(NULL)
, routineClock_(0)
, placeStoneClock_(0)
, exit_(false)
, placePos_(-1)
{
	std::fill(board_.begin(), board_.end(), 0);
}

BoardSpy::~BoardSpy() {

	send_command("quit");

	if (hDisplayDC_)
		ReleaseDC(NULL, hDisplayDC_);
}



static 
BOOL CALLBACK Level0EnumWindowsProc(HWND hWnd, LPARAM lParam) {

	RECT rc;
	GetWindowRect(hWnd, &rc);

	if ((rc.right - rc.left < min_hwnd_pixes) || (rc.bottom - rc.top < min_hwnd_pixes)) {
		// too small , cant be
		return TRUE;
	}

	BoardSpy* spy = (BoardSpy*)lParam;
	if (spy->bindWindow(hWnd))
		return FALSE; // end search

	return TRUE;
}

void BoardSpy::exit() {
	exit_ = true;
}

void BoardSpy::initResource() {

	if (!initBitmaps())
		fatal(_T("resource bitmaps fail"));

	// detect display device
	hDisplayDC_ = CreateDC(_T("display"), NULL, NULL, NULL);
	auto ibits = GetDeviceCaps(hDisplayDC_, BITSPIXEL)*GetDeviceCaps(hDisplayDC_, PLANES);

	if (ibits <= 16) {
		fatal(_T("not supported display device"));
	}

	if (ibits <= 24)
		nDispalyBitsCount_ = 24;
	else
		nDispalyBitsCount_ = 32;

	nBpp_ = nDispalyBitsCount_ >> 3;

	char program_path[1024];
	HMODULE hModule = GetModuleHandle(NULL);
	GetModuleFileNameA(hModule, program_path, 1024);
	int last = (int)strlen(program_path);
	while (last--){
		if (program_path[last] == '\\' || program_path[last] == '/') {
			program_path[last] = '\0';
			break;
		}
	}

	init(program_path);

	std::thread([&]() {

		while (!exit_) {

			if (hTargetWnd == NULL) {
				
				// Make a callback procedure for Windows to use to iterate
				// through the Window list
				FARPROC EnumProcInstance = MakeProcInstance((FARPROC)Level0EnumWindowsProc, g_hInst);
				// Call the EnumWindows function to start the iteration
				EnumWindows((WNDENUMPROC)EnumProcInstance, (LPARAM)this);
				
				// Free up the allocated memory handle
				FreeProcInstance(EnumProcInstance);
			}

			Sleep(1000);
		}

	}).detach();

	if (onGtpReady)
		onGtpReady();
}


bool BoardSpy::initBitmaps() {

	TCHAR resdir[256];
	TCHAR path[256];

	lstrcpy(resdir, g_szLocalPath);
	lstrcat(resdir, _T("\\res\\"));

	lstrcpy(path, resdir);
	lstrcat(path, "black.bmp");
	int w, h;
	if (!loadBmpData(path, blackStoneData_, w, h))
		return false;
		
	tplStoneSize_ = w;

	lstrcpy(path, resdir);
	lstrcat(path, "white.bmp");
	if (!loadBmpData(path, whiteStoneData_, w, h))
		return false;
	
	if (tplStoneSize_ != w || tplStoneSize_ != h)
		return false;

	tplBoardSize_ = tplStoneSize_ * 19;

	blackImage_.resize(tplStoneSize_*tplStoneSize_*3, 0);
	whiteImage_.resize(tplStoneSize_*tplStoneSize_ * 3, 255);

	// load stone mask
	stoneMaskData_.resize(tplStoneSize_*tplStoneSize_);
	std::fill(stoneMaskData_.begin(), stoneMaskData_.end(), 1);

	for (auto n = 0; n < sizeof(mask_files) / sizeof(LPCTSTR); ++n) {
		lstrcpy(path, resdir);
		lstrcat(path, mask_files[n]);

		std::vector<BYTE> data;
		int width, height;
		if (!loadBmpData(path, data, width, height))
			return false;

		if (width != tplStoneSize_ || height != tplStoneSize_)
			return false;

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++) {
				auto idx = i*width + j;
				auto target = i*width * 3 + j * 3;
				auto r = data[target];
				auto g = data[target + 1];
				auto b = data[target + 2];

				if (r != 0 || g != 0 || b != 255) {
					stoneMaskData_[idx] = 0;
				}
			}
		}
	}

	lastMoveMaskData_.resize(tplStoneSize_*tplStoneSize_);
	std::fill(lastMoveMaskData_.begin(), lastMoveMaskData_.end(), 0);
	lstrcpy(path, resdir);
	lstrcat(path, mask_files[0]);

	std::vector<BYTE> data;
	int width, height;
	if (!loadBmpData(path, data, width, height))
		return false;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++) {
			auto idx = i*width + j;
			auto target = i*width * 3 + j * 3;
			auto r = data[target];
			auto g = data[target + 1];
			auto b = data[target + 2];

			if (r == 255 && g == 0 && b == 0) {
				lastMoveMaskData_[idx] = 1;
			}
		}
	}


	return true;
}


int BoardSpy::detectStone(const BYTE* DIBS, int move, bool& isLastMove) const {

	isLastMove = false;

	bool is_black = false, is_white = false;
	auto bratio = compareBoardRegionAt(DIBS, move, blackStoneData_, stoneMaskData_);
	if (bratio > 0.9) {
		is_black = true;
	} else {
		auto wratio = compareBoardRegionAt(DIBS, move, whiteStoneData_, stoneMaskData_);
		if (wratio > 0.9) {
			is_white = true;
		}
	}

	int sel = 0;

	if (is_black) {
		sel = 1;
		auto ratio = compareBoardRegionAt(DIBS, move, whiteImage_, lastMoveMaskData_);
		isLastMove = (ratio > 0.9);
	} else if (is_white) {
		sel = -1;
		auto ratio = compareBoardRegionAt(DIBS, move, blackImage_, lastMoveMaskData_);
		isLastMove = (ratio > 0.9);
	}

	return sel;
}

double BoardSpy::compareBoardRegionAt(const BYTE* DIBS, int idx, const std::vector<BYTE>& stone, const std::vector<BYTE>& mask) const {

	auto left = tplStoneSize_*(idx%19);
	auto top = tplStoneSize_*(idx/19);

	auto bottom = top + tplStoneSize_;
	auto right = left + tplStoneSize_;

	
	auto biWidthBytes = ((tplBoardSize_ * nDispalyBitsCount_ + 31) & ~31) / 8;

	int64_t totalLast = 0;
	double total = 0;


	int counts = 0;
	auto pmask = &mask[0];

	const BYTE* pstone = &stone[0];
	for (auto i = top; i < bottom; i++)
	{
		auto y = 25 * 19 - i - 1;
		for (int j = left; j < right; j++) {

			auto idx_m = (i - top)*tplStoneSize_ + (j - left);
			if (!pmask[idx_m]) continue;

			counts++;
			
			auto idx = y*biWidthBytes + j*nBpp_;
			auto r = DIBS[idx + 2];
			auto g = DIBS[idx + 1];
			auto b = DIBS[idx];
			
			auto idx2 = (i - top)*tplStoneSize_ * 3 + (j - left) * 3;
			auto r2 = pstone[idx2];
			auto g2 = pstone[idx2 + 1];
			auto b2 = pstone[idx2 + 2];
			
			//total += abs(r - r2) + abs(g - g2) + abs(b - b2);
			total += square_diff(RGB(r, g, b), RGB(r2, g2, b2));
		}
	}

	double diff = (double)total / (double)counts;
	return diff;
}

bool BoardSpy::scanBoard(HWND hWnd, int data[], int& lastMove) {

	Hdc hdc;
	hdc = GetWindowDC(hWnd);

	// copy screen to bitmap
	if (stoneSize_ != tplStoneSize_) {
		
		StretchBlt(hBoardDC_, 0, 0, tplBoardSize_, tplBoardSize_, hdc, offsetX_, offsetY_, stoneSize_ * 19, stoneSize_ * 19, SRCCOPY);
	}
	else {
		BitBlt(hBoardDC_, 0, 0, tplBoardSize_, tplBoardSize_, hdc, offsetX_, offsetY_, SRCCOPY);
	}
		
	boardDIB_.getFromBitmap(hDisplayDC_, nDispalyBitsCount_, hBoardBitmap_, tplBoardSize_, tplBoardSize_);

	lastMove = -1;
	for (auto idx = 0; idx < 361; idx++) {
		bool isLastMove;
		auto stone = detectStone(boardDIB_.data(), idx, isLastMove);
		data[idx] = stone;

		if (isLastMove)
			lastMove = idx;
	}

	return true;
}



bool BoardSpy::locateStartPosition(Hdib& hdib, int& startx, int& starty) {

	// skip title bar
	if (hdib.width() < 50 || hdib.height() < 50)
		return false;

	for (int i=30; i<50; i++) {
		for (int j=2; j<50; j++) {
			if (square_diff(hdib.rgb(j,i), RGB(60,60,60)) > 0.98) {
				startx = j;
				starty = i;
				return true;
			}
		}
	}
	return false;
}

bool BoardSpy::calcBoardPositions(HWND hWnd, int startx, int starty) {

	Hdib hdib;
	hdib.getFromWnd(hWnd, nDispalyBitsCount_);

	if (startx < 0 || starty < 0) {
		if (!locateStartPosition(hdib, startx, starty))
			return false;
	}

	bool found = false;
	int tx, ty;
	for (int i=starty; i < 600; i++) {
		for (int j=startx; j<600; j++) {
			if (square_diff(hdib.rgb(j,i), RGB(60,60,60)) < 0.8) {
				found = true;
				tx = j;
				ty = i;
				break;
			}
		}
		if (found)
			break;
	}

	if (!found)
		return false;

	int boardh = 0, boardw = 1;
	for (int i=ty+1; i<hdib.height(); i++) {
			if (square_diff(hdib.rgb(tx,i), RGB(60,60,60)) > 0.9) {
				boardh = i - ty;
				break;
			}
	}
	
	for (int i=tx+1; i<hdib.width(); i++) {
			if (square_diff(hdib.rgb(i,ty), RGB(60,60,60)) > 0.9) {
				boardw = i - tx;
				break;
			}
	}
		
	if (boardh != boardw)
		return false;

	// create only once
	if (!hBoardBitmap_) {
			
		hBoardDC_ = CreateCompatibleDC(hDisplayDC_);
		SetStretchBltMode(hBoardDC_, HALFTONE);
			
		hBoardBitmap_ = CreateCompatibleBitmap(hDisplayDC_, tplBoardSize_, tplBoardSize_);
		hBoardDC_.selectBitmap(hBoardBitmap_);
	}
	
	offsetX_ = tx + 1;
	offsetY_ = ty + 1;
	stoneSize_ = boardw / 19;

	return true;
}



bool BoardSpy::bindWindow(HWND hWnd) {

	Hdib hdib;
	int startx, starty;
	hdib.getFromWnd(hWnd, nDispalyBitsCount_, 50, 50);
	if (!locateStartPosition(hdib, startx, starty))
		return false;
	
	if (calcBoardPositions(hWnd, startx, starty)) {
		
		if (!initialBoard(hWnd))
			return false;

		hTargetWnd = hWnd;
		GetWindowRect(hWnd, &lastRect_);
		return true;
	}

	return false;
}

bool BoardSpy::routineCheck() {

	routineClock_++;

	pop_events();

	if (!IsWindow(hTargetWnd)) {

		hTargetWnd = NULL;
		return false;
	} else {

		if (!calcBoardPositions(hTargetWnd, -1, -1)) {
			if (error_count_++ > 10) {
				// board closed
				hTargetWnd = NULL;
				return false;
			}
			return true;
		}

		RECT rc;
		GetWindowRect(hTargetWnd, &rc);

		if (lastRect_.left != rc.left ||
			lastRect_.top != rc.top ||
			lastRect_.right != rc.right ||
			lastRect_.bottom != rc.bottom) {

			lastRect_ = rc;

			if (onSizeChanged)
				onSizeChanged();
		}	
	}

	int curBoard[361];
	int lastMove;
	if (!scanBoard(hTargetWnd, curBoard, lastMove)) {

		if (error_count_++ > 10) {
			hTargetWnd = NULL;
			return false;
		}
		return true;
	}

	error_count_ = 0;

	auto thres = THRESH_STONE_EXISTS_INTERVAL;

	int sel = -1;
	std::vector<int> candicates;
	for (auto idx = 0; idx < 361; idx++) {

		auto stone = curBoard[idx];
		auto old = board_[idx];

		if (old != stone) {
			board_age[idx]=0;
			board_[idx] = stone;
		}
		else {
			board_age[idx]++;
			if (board_age[idx] > thres) {
				if (stone != 0 && board_last_[idx]== 0) {
					candicates.push_back(idx);
				}
			}
		}
	}

	if (candicates.size()) {
		auto mLast = -1;
		auto mTurn = -1;
		auto mOther = -1;
		for (auto c : candicates) {
			if (c == lastMove) mLast = c;
			else if (board_[c] == (next_move_is_black()?1:-1)) mTurn = c;
			else mOther = c;
		}

		if (mTurn >= 0) sel = mTurn;
		else if (mOther >= 0) sel = mOther;
		else sel = mLast;
	}

	if (sel >= 0) {

		placeStoneClock_ = 0;

		auto player = board_[sel];
		board_last_[sel] = player;

		for (int idx = 0; idx < 361; idx++) {
			if (board_age[idx] > thres && board_[idx]== 0) {
				board_last_[idx] = 0;
			}
		}

		std::copy(board_last_.begin(), board_last_.end(), board_.begin());
		memset(board_age, 0, sizeof(board_age));

		place(player == 1, sel);
	}

	if (placeStoneClock_ > 0 && placeStoneClock_ < routineClock_) {
		if (placeStone)
			placeStone(placeX_, placeY_);
	}
	
	return true;
}


bool BoardSpy::initialBoard(HWND hWnd) {

	int curBoard[361];
	int lastMove;
	if (!scanBoard(hWnd, curBoard, lastMove))
		return false;

	int more_stones = 0;
	int stone_counts = 0;
	for (auto idx=0; idx<361; idx++) {
		if (curBoard[idx] != 0) {
			stone_counts++;
			if (board_[idx] == 0)
				more_stones++;
		}
	}

	//char buf[1024];
	//sprintf(buf, "%d", more_stones);

	//fatal(buf);
	if (more_stones > 1)
		return false;

	if (stone_counts <= 1) {
		std::fill(board_.begin(), board_.end(), 0);
		std::fill(board_last_.begin(), board_last_.end(), 0);
		memset(board_age, 0, sizeof(board_age));
	
		//reset(stone_counts == 1 ? false : true);
		reset(false);
	}

	return true;
}

bool BoardSpy::found() const {
	return IsWindow(hTargetWnd);
}

POINT BoardSpy::coord2Screen(int x, int y) const {

	POINT pt;
	pt.x = 0;
	pt.y = 0;

	if (!found())
		return pt;

	RECT rc;
	GetWindowRect(hTargetWnd, &rc);
	pt.y = rc.top + offsetY_ + y*stoneSize_ + stoneSize_ / 2;
	pt.x = rc.left + offsetX_ + x*stoneSize_ + stoneSize_ / 2;
	return pt;
}



void BoardSpy::placeAt(int x, int y) {

	if (!found()) {
		placeStoneClock_ = 0;
		return;
	}

	POINT at = coord2Screen(x, y);

	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(at.x, at.y);

	mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, NULL);
	Sleep(30);
	mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, NULL);

	SetCursorPos(pt.x, pt.y);

	placeX_ = x;
	placeY_ = y;
	placeStoneClock_ = routineClock_ + 50;

	/*
	if (!found())
		return;

	int Y = offsetY_ + y*stoneSize_ + stoneSize_ / 2;
	int X = offsetX_ + x*stoneSize_ + stoneSize_ / 2;
	int lparam =  (Y << 16) +  X;
	
	::PostMessage(hTargetWnd, WM_LBUTTONDOWN, 0, lparam);
	Sleep(100);
	::PostMessage(hTargetWnd, WM_LBUTTONUP, 0, lparam);

SendMessage(hTargetWnd, WM_MOUSEMOVE, 0, MAKELPARAM(X, Y));
	Sleep(100);
	SendMessage(hTargetWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(X, Y));
	Sleep(50);
	SendMessage(hTargetWnd, WM_LBUTTONUP, 0, MAKELPARAM(X, Y));
	Sleep(100);
	*/
}


inline void trim(std::string &ss)   
{   
	auto p=find_if(ss.rbegin(),ss.rend(),std::not1(std::ptr_fun(isspace)));   
    ss.erase(p.base(),ss.end());  
	auto p2 = std::find_if(ss.begin(),ss.end(),std::not1(std::ptr_fun(isspace)));   
	ss.erase(ss.begin(), p2);
} 

void BoardSpy::init(const std::string& cfg_path) {

	GTP::setup_default_parameters();
	cfg_max_visits = 3200;

	vector<string> players;
	parseLeelaZeroArgs(__argc, __argv, players);

    if (players.empty()) {
        fatal("A network weights file is required to use the program");
    }

	set_init_cmds({"time_settings 1800 15 1"});
	
	try {
		if (players[0].empty())
			execute();
		else
			execute(players[0], cfg_path, 40);
		if (!isReady()) 
			throw std::runtime_error("cannot execute engine");
		
		if (!support("undo"))
			throw std::runtime_error("the engine do not support undo command");
		
	} catch(std::exception& e) {
		fatal(e.what());
		return;
	}
}


// 902