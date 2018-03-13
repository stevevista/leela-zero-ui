#pragma once
#include <vector>
#include <algorithm>
#include <windows.h>

class Lock {
	CRITICAL_SECTION cs_;
public:
	Lock() {
		InitializeCriticalSection(&cs_);
	}
	~Lock() {
		DeleteCriticalSection(&cs_);
	}
	void lock() {
		EnterCriticalSection(&cs_);
	}
	void unlock() {
		LeaveCriticalSection(&cs_);
	}
};

class Hdc {
	HDC h_;
	HBITMAP bmOld_;
public:
	Hdc() : h_(NULL), bmOld_(NULL) {}
	~Hdc() {
		if (h_) {
			if (bmOld_) SelectObject(h_, bmOld_);
			DeleteDC(h_);
		}
	}

	operator bool() const { return h_ != NULL; }
	operator HDC() { return h_; }
	Hdc& operator=(HDC h) {

		if (h_) {
			if (bmOld_) SelectObject(h_, bmOld_);
			DeleteDC(h_);
		}
		h_ = h;
		bmOld_ = NULL;
		return *this;
	}

	void selectBitmap(HBITMAP bmp) {
		auto old = SelectObject(h_, bmp);
		if (bmOld_ == NULL)
			bmOld_ = (HBITMAP)old;
	}
};

class HBitmap {
	HBITMAP h_;
public:
	HBitmap() :h_(NULL) {}
	~HBitmap() {
		if (h_) DeleteObject(h_);
	}
	operator bool() const { return h_ != NULL; }
	operator HBITMAP() { return h_; }
	operator const HBITMAP() const { return h_; }
	HBitmap& operator=(HBITMAP h) {

		if (h_) DeleteObject(h_);
		h_ = h;
		return *this;
	}

	void copyToClipboard() {
		OpenClipboard(NULL);
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, h_);
		CloseClipboard();
	}
};


class HMemDC {
	HBitmap hbitmap_;
	Hdc hdc_;
	int w_;
	int h_;
public:
	void create(HDC hdc, int width, int height) {
		w_ = width;
		h_ = height;
		hdc_ = CreateCompatibleDC(hdc);
		hbitmap_ = CreateCompatibleBitmap(hdc, width, height);
		hdc_.selectBitmap(hbitmap_);
	}

	void bitBlt(HDC hdc, int left, int top) {
		BitBlt(hdc_, 0, 0, w_, h_, hdc, left, top, SRCCOPY);
	}

	void bitBlt(HWND hwnd, int left, int top) {

		Hdc hdc;
		hdc = GetWindowDC(hwnd);
		RECT rc;
		GetWindowRect(hwnd, &rc);
	
		auto width = rc.right - rc.left;
		auto height = rc.bottom - rc.top;
		create(hdc, width, height);
		bitBlt(hdc, left, top);
	}

	const HBitmap& bitmap() const { return hbitmap_; }

	void copyToClipboard() {
		hbitmap_.copyToClipboard();
	}
	
};

class Hdib {
	std::vector<BYTE> bits_;
	int w_;
	int h_;
	BITMAPINFO info_;
	int bpp_;
	int biWidthBytes_;

public:

	int width() const { return w_; }
	int height() const { return h_; }

	void getFromBitmap(HDC hdc, int bitscount, const HBitmap& bitmap, int w, int h) {

		bpp_ = bitscount >> 3;
		biWidthBytes_ = ((w * bitscount + 31) & ~31) / 8;

		w_ = w;
		h_ = h;
		if (bits_.size() < w_*h_*bpp_)
			bits_.resize(w_*h_*bpp_);

		
		info_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info_.bmiHeader.biWidth = w_; // targetWidth
		info_.bmiHeader.biHeight = h_; //targetHeight
		info_.bmiHeader.biPlanes = 1;
		info_.bmiHeader.biBitCount = bitscount;
		info_.bmiHeader.biClrImportant = 0;
		info_.bmiHeader.biClrUsed = 0;
		info_.bmiHeader.biCompression = BI_RGB;
		info_.bmiHeader.biSizeImage = 0;
		info_.bmiHeader.biYPelsPerMeter = 0;
		info_.bmiHeader.biXPelsPerMeter = 0;
		GetDIBits(hdc, bitmap, 0, (UINT)h, &bits_[0], &info_, DIB_RGB_COLORS);
	}

	void getFromWnd(HWND hwnd, int bitscount, int w=-1, int h=-1) {

		RECT rc;
		GetWindowRect(hwnd, &rc);
		auto width = rc.right - rc.left;
		auto height = rc.bottom - rc.top;

		if (w < 0 || w > width) w = width;
		if (h < 0 || h > height) h = height;

		Hdc memdc;
		HBitmap hbitmap;
		Hdc hdc;
		hdc = GetWindowDC(hwnd);
		memdc = CreateCompatibleDC(hdc);
		hbitmap = CreateCompatibleBitmap(hdc, w, h);
		memdc.selectBitmap(hbitmap);

		BitBlt(memdc, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
		getFromBitmap(hdc, bitscount, hbitmap, w, h);
	}


	const BYTE* data() const { return &bits_[0]; }

	COLORREF rgb(int x, int y) const {
		auto idx = (h_-1-y)*biWidthBytes_ + x*bpp_;
		auto r = bits_[idx + 2];
		auto g = bits_[idx + 1];
		auto b = bits_[idx];
		return RGB(r, g, b); 
	}

	void save(const std::string& path);

};

