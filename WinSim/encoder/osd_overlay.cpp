#include "osd_overlay.h"

#include <algorithm>
#include <ctype.h>
#include <string.h>

static void SetPixelNv12(uint8_t* nv12,
	int x,
	int y,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	const YuvColor& color)
{
	if (nv12 == NULL || x < 0 || y < 0 || x >= width || y >= height)
	{
		return;
	}

	uint8_t* y_plane = nv12;
	uint8_t* uv_plane = nv12 + static_cast<size_t>(hor_stride) * static_cast<size_t>(ver_stride);
	y_plane[static_cast<size_t>(y) * static_cast<size_t>(hor_stride) + x] = color.y;

	const int uv_x = x & ~1;
	const int uv_y = y / 2;
	uv_plane[static_cast<size_t>(uv_y) * static_cast<size_t>(hor_stride) + uv_x] = color.u;
	uv_plane[static_cast<size_t>(uv_y) * static_cast<size_t>(hor_stride) + uv_x + 1] = color.v;
}

static void FillRectNv12(uint8_t* nv12,
	int x,
	int y,
	int w,
	int h,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	const YuvColor& color)
{
	const int x0 = std::max(0, x);
	const int y0 = std::max(0, y);
	const int x1 = std::min(width, x + w);
	const int y1 = std::min(height, y + h);

	for (int yy = y0; yy < y1; ++yy)
	{
		for (int xx = x0; xx < x1; ++xx)
		{
			SetPixelNv12(nv12, xx, yy, width, height, hor_stride, ver_stride, color);
		}
	}
}

static void DrawRectNv12(uint8_t* nv12,
	int x,
	int y,
	int w,
	int h,
	int line_width,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	const YuvColor& color)
{
	line_width = std::max(1, line_width);
	FillRectNv12(nv12, x, y, w, line_width, width, height, hor_stride, ver_stride, color);
	FillRectNv12(nv12, x, y + h - line_width, w, line_width, width, height, hor_stride, ver_stride, color);
	FillRectNv12(nv12, x, y, line_width, h, width, height, hor_stride, ver_stride, color);
	FillRectNv12(nv12, x + w - line_width, y, line_width, h, width, height, hor_stride, ver_stride, color);
}

static uint8_t GlyphRows(char c, int row)
{
	static const uint8_t digits[10][7] = {
		{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
		{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
		{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
		{0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E},
		{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
		{0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E},
		{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
		{0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
		{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
		{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
	};
	static const uint8_t letters[26][7] = {
		{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
		{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
		{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
		{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
		{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
		{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
		{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
		{0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
		{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
		{0x07,0x02,0x02,0x02,0x12,0x12,0x0C},
		{0x11,0x12,0x14,0x18,0x14,0x12,0x11},
		{0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
		{0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
		{0x11,0x19,0x15,0x13,0x11,0x11,0x11},
		{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
		{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
		{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
		{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
		{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
		{0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
		{0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
		{0x11,0x11,0x11,0x0A,0x0A,0x04,0x04},
		{0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
		{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
		{0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
		{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
	};

	if (row < 0 || row >= 7)
	{
		return 0;
	}
	if (c >= '0' && c <= '9')
	{
		return digits[c - '0'][row];
	}
	c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
	if (c >= 'A' && c <= 'Z')
	{
		return letters[c - 'A'][row];
	}
	if (c == '-')
	{
		return row == 3 ? 0x1F : 0;
	}
	if (c == '_')
	{
		return row == 6 ? 0x1F : 0;
	}
	return 0;
}

static void DrawTextNv12(uint8_t* nv12,
	int x,
	int y,
	const std::string& text,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	const YuvColor& color)
{
	int cursor_x = x;
	for (size_t i = 0; i < text.size(); ++i)
	{
		const char c = text[i];
		for (int row = 0; row < 7; ++row)
		{
			const uint8_t bits = GlyphRows(c, row);
			for (int col = 0; col < 5; ++col)
			{
				if (bits & (1 << (4 - col)))
				{
					SetPixelNv12(nv12, cursor_x + col, y + row, width, height, hor_stride, ver_stride, color);
				}
			}
		}
		cursor_x += 6;
	}
}

void DrawOsdNv12(uint8_t* nv12,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	const std::vector<OsdObject>& objects)
{
	const YuvColor bg = {16, 128, 128};

	for (size_t i = 0; i < objects.size(); ++i)
	{
		const OsdObject& obj = objects[i];
		DrawRectNv12(nv12, obj.x, obj.y, obj.w, obj.h, obj.line_width,
			width, height, hor_stride, ver_stride, obj.color);

		if (!obj.label.empty())
		{
			const int text_w = static_cast<int>(obj.label.size()) * 6 + 4;
			const int text_h = 11;
			const int label_y = std::max(0, obj.y - text_h);
			FillRectNv12(nv12, obj.x, label_y, text_w, text_h, width, height, hor_stride, ver_stride, bg);
			DrawTextNv12(nv12, obj.x + 2, label_y + 2, obj.label, width, height, hor_stride, ver_stride, obj.color);
		}
	}
}

std::vector<OsdObject> MakeTestOsdObjects(int width, int height)
{
	std::vector<OsdObject> objects;
	OsdObject a;
	a.x = width / 8;
	a.y = height / 8;
	a.w = width / 4;
	a.h = height / 4;
	a.label = "OBJ-1";
	a.color = {235, 16, 240};
	a.line_width = 3;
	objects.push_back(a);

	OsdObject b;
	b.x = width / 2;
	b.y = height / 3;
	b.w = width / 5;
	b.h = height / 5;
	b.label = "OBJ-2";
	b.color = {210, 90, 54};
	b.line_width = 3;
	objects.push_back(b);

	return objects;
}
