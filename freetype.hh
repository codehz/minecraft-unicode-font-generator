#ifndef FREETYPE_HH
#define FREETYPE_HH

#include <string>
#include <cstring>
#include <map>
#include <exception>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "create_map.hh"
#include "range_map.hh"

extern "C" 
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
}

class FTError : public std::runtime_error {
	char buffer[64];
public:
	FTError(int err) : std::runtime_error(buffer) {
		sprintf(buffer, "FreeTypeError: %d", err);
	}

	FTError(int err, std::map<int, std::string> errmap) : std::runtime_error(buffer) {
		auto it = errmap.find(err);
		if (it != errmap.end()) sprintf(buffer, "FreeTypeError: %d(%s)", err, it->second.c_str());
		else sprintf(buffer, "FreeTypeError: %d", err);
	}

	char *what() {
		return buffer;
	}
};

void CatchFTError(FT_Error err) {
	if (err == 0) return;
	throw FTError(err);
}

template <typename ...Type>
void CatchFTError(FT_Error err, Type ...params) {
	if (err == 0) return;
	throw FTError(err, params...);
}

class Bitmap {
	unsigned *data;
	unsigned width;
public:
	Bitmap(unsigned width) : data(new unsigned[width * width]), width(width) {
	}

	~Bitmap() {
		delete[] data;
	}

	unsigned getWidth() const {
		return width;
	}

	struct Rows {
		unsigned *data;
		unsigned offset;
		unsigned &operator [] (unsigned index) const {
			return data[offset + index];
		}
	};

	const Rows operator [] (unsigned index) const {
		return {data, index * width};
	}
};

struct SimpleColorConverter {
	unsigned operator () (unsigned char in) {
		return 0xFF000000 + ((unsigned)in << 16) + ((unsigned)in << 8) + in;
	}
};

const range_map<unsigned, bool> fwrange{make_range(0x3000, 0x30FF),make_range(0x4e00, 0x9faf),make_range(0xff00, 0xffef)};
const int light_map[] = {FT_LOAD_TARGET_NORMAL, FT_LOAD_TARGET_LIGHT, FT_LOAD_TARGET_MONO};
bool checkFTParams(int light_mode) {return light_mode < 0 || light_mode >= sizeof(light_map) / sizeof(*light_map);}

template <typename bitmap_t = Bitmap, typename colorConverter_t = SimpleColorConverter>
class FreeType {
private:
	FT_Library library;
	FT_Face	   face;
	bitmap_t   *bitmap;
	unsigned   pixel;
	int        baseline;
	colorConverter_t colorConverter;
	const int  render_mode;
	FT_Bitmap  *cvt_bitmap;
public:
	FreeType(bitmap_t *bitmap, std::string fontname, int face_index, unsigned pixel, unsigned fontsize, int light_mode) : bitmap(bitmap), pixel(pixel), render_mode(light_map[light_mode]) {
		CatchFTError(FT_Init_FreeType(&library));
		CatchFTError(FT_New_Face(library, fontname.c_str(), face_index, &face), create_map<int, std::string>(FT_Err_Unknown_File_Format, "Unknown File Format"));
		CatchFTError(FT_Set_Pixel_Sizes(face, 0, fontsize));
		FT_Bitmap_Init(cvt_bitmap);
		std::cout << "Font " << face->family_name << "(" << face->style_name << ") Loaded. base line:" << (baseline = abs(face->descender) * fontsize / face->units_per_EM) << std::endl;
	}

	~FreeType() {
		FT_Bitmap_Done(library, cvt_bitmap);
	}

	bitmap_t *genStart(unsigned long from, unsigned char *glyph_info) {
		for (unsigned i = 0; i < 16; i++)
			for (unsigned j = 0; j < 16; j++) {
				auto ch = from * 256 + i * 16 + j;
				genChar(ch);
				drawTo(i, j, glyph_info + ch, fwrange.find(make_single_range(ch)) != fwrange.end());
			}
		return bitmap;
	}

	void genChar(unsigned long c) {
		auto index = FT_Get_Char_Index(face, c);
		CatchFTError(FT_Load_Glyph(face, index, FT_LOAD_RENDER | render_mode));
	}

	void drawTo(unsigned x, unsigned y, unsigned char *glyph_info, bool fw = false) {
		auto buffer = face->glyph->bitmap.buffer;
		if (buffer == nullptr) return;
		auto cols = std::min<int>(face->glyph->bitmap.width, pixel);
		auto rows = std::min<int>(face->glyph->bitmap.rows, pixel);
		auto xoffset = std::max<int>(pixel - (baseline + face->glyph->metrics.horiBearingY / 64), 0);
		auto yoffset = std::max<int>(face->glyph->bitmap_left, 0);
		xoffset -= std::max<int>(xoffset + rows - pixel, 0);
		yoffset -= std::max<int>(yoffset + cols - pixel, 0);
		*glyph_info = fw ? 0x0f : ((unsigned char)std::min<int>(std::floor((float)yoffset * 0xF / pixel), 0xF) * 16 + (unsigned char)std::min<int>(std::ceil(((float)yoffset + cols) * 0xF / pixel), 0xF));
#ifdef DEBUG
		std::cerr << x << "," << y << ":[" << cols << "," << rows << "]" << "<" << xoffset << "," << yoffset << ">" << "(" << (float)yoffset * 0xF / pixel << "," << ((float)yoffset + cols) * 0xF / pixel << ")" << std::setw(2) << std::setfill('0') << std::hex <<  (int)*glyph_info << std::dec << "=" << (int)face->glyph->bitmap.pixel_mode << std::endl;
#endif
		if (face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
			unsigned char *row;
			for (unsigned i = 0; i < rows; i++) {
				for (unsigned j = 0; j < cols; j++)
					(*bitmap)[x * pixel + i + xoffset][y * pixel + j + yoffset] = colorConverter((!!(buffer[i * face->glyph->bitmap.pitch + (j >> 3)] & (128 >> (j & 7)))) * 0xFF);
			}
		}
		else if (face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
	 		for (unsigned i = 0; i < rows; i++)
				for (unsigned j = 0; j < cols; j++)
					(*bitmap)[x * pixel + i + xoffset][y * pixel + j + yoffset] = colorConverter(buffer[i * face->glyph->bitmap.pitch + j]);
	}
};

#endif
