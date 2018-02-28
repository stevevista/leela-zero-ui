#include "spy.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_INLINE
#include "stb/stb_image_write.h"

void fatal(LPCTSTR msg) {
	MessageBox(NULL, msg, TEXT("spy"),
		MB_ICONERROR);
	exit(-1);
}


bool loadBmpData(LPCTSTR pszFile, std::vector<BYTE>& img, int& width, int& height) {

	width = 0;
	height = 0;

	DWORD dwTmp;

	// Create the .BMP file.  
	HANDLE hf = CreateFile(pszFile,
		GENERIC_READ,
		(DWORD)0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)NULL);

	if (hf == INVALID_HANDLE_VALUE) {
		return false;
	}

	BITMAPFILEHEADER hdr;
	BITMAPINFOHEADER bih;

	// Copy the BITMAPFILEHEADER into the .BMP file.  
	if (!ReadFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER),
		(LPDWORD)&dwTmp, NULL))
	{
		CloseHandle(hf);
		return false;
	}

	if (!ReadFile(hf, (LPVOID)&bih, sizeof(BITMAPINFOHEADER),
		(LPDWORD)&dwTmp, NULL))
	{
		CloseHandle(hf);
		return false;
	}


	std::vector<BYTE> raw(bih.biSizeImage);

	if (!ReadFile(hf, (LPSTR)&raw[0], raw.size(),
		(LPDWORD)&dwTmp, NULL))
	{
		CloseHandle(hf);
		return false;
	}

	width = bih.biWidth;
	height = bih.biHeight;
	img.resize(width*height * 3);

	const int Bpp = bih.biBitCount >> 3;
	auto widthbytes = bih.biWidth* Bpp;
	auto biWidthBytes = ((bih.biWidth * bih.biBitCount + 31) & ~31) / 8;

	for (int i = 0; i < bih.biHeight; i++)
	{
		auto y = bih.biHeight - i - 1;
		for (int j = 0; j < bih.biWidth; j++) {
			auto idx = y*biWidthBytes + j*Bpp;
			auto target = i*width * 3 + j * 3;
			auto r = raw[idx + 2];
			auto g = raw[idx + 1];
			auto b = raw[idx];
			img[target] = r;
			img[target + 1] = g;
			img[target + 2] = b;
		}
	}

	CloseHandle(hf);
	return true;
}



void Hdib::save(const std::string& path) {

	std::vector<char> bytes(w_*h_*3);
	auto dst = &bytes[0];
	for (int y=0; y<h_; y++) {
		for (int x=0; x<w_; x++) {
			auto c = rgb(x,y);
			*dst++ = GetRValue(c);
			*dst++ = GetGValue(c);
			*dst++ = GetBValue(c);
		}
	}

	stbi_write_bmp(path.c_str(), w_, h_, 3, &bytes[0]);
}

void save_dib(const std::string& path, const BYTE* data, int size, int bw, int bpp) {

	std::vector<char> bytes(size*size*3);
	auto dst = &bytes[0];
	for (int y=0; y<size; y++) {
		for (int x=0; x<size; x++) {
			auto idx = (size-1-y)*bw + x*bpp;
			*dst++ = data[idx+2];
			*dst++ = data[idx+1];
			*dst++ = data[idx];
		}
	}
	stbi_write_bmp(path.c_str(), size, size, 3, &bytes[0]);

}
