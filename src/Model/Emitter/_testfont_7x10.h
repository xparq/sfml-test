#ifndef _CNXV4IU5YTB9678DT5674BTN6893476T0576FYM4780NBU87Y_
#define _CNXV4IU5YTB9678DT5674BTN6893476T0576FYM4780NBU87Y_

static constexpr unsigned font_height = 10;
static constexpr unsigned font_width = 7;
static constexpr unsigned font_baseline_index = 2; // From the bottom (0-based)!

static constexpr uint8_t font[] = {
			//!! Allow optional per-char sizes! -- Would prevent simple glyph * height direct indexing tho!

// ' '
			0,0,0,0,0,0,0,0,0,0,
// !
			0b0000000,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0000000,
			0b0010000,
			0b0000000,
			0b0000000,
// "
			0b0000000,
			0b0000000,
			0b0010100,
			0b0010100,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
// #
			0b0000000,
			0b0000000,
			0b0000000,
			0b0010100,
			0b0111110,
			0b0010100,
			0b0111110,
			0b0010100,
			0b0000000,
			0b0000000,
// $
			0b0000000,
			0b0001000,
			0b0011110,
			0b0101000,
			0b0011100,
			0b0001010,
			0b0001010,
			0b0111100,
			0b0001000,
			0b0000000,
// %
			0b0000000,
			0b0000000,
			0b0100001,
			0b0000010,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0100001,
			0b0000000,
			0b0000000,
// &
			0b0000000,
			0b0000000,
			0b0011000,
			0b0100100,
			0b0011000,
			0b0101010,
			0b1000100,
			0b0111010,
			0b0000000,
			0b0000000,
// '
			0b0000000,
			0b0000000,
			0b0001000,
			0b0001000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
// (
			0b0000000,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0001000,
			0b0000100,
			0b0000000,
// )
			0b0000000,
			0b0010000,
			0b0001000,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0000000,
// *
			0b0000000,
			0b0000000,
			0b0000000,
			0b0010100,
			0b0001000,
			0b0111110,
			0b0001000,
			0b0010100,
			0b0000000,
			0b0000000,
// +
			0b0000000,
			0b0000000,
			0b0000000,
			0b0001000,
			0b0001000,
			0b0111110,
			0b0001000,
			0b0001000,
			0b0000000,
			0b0000000,
// ,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0001000,
			0b0010000,
			0b0000000,
			0b0000000,
// -
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0111110,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
// .
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0001000,
			0b0000000,
			0b0000000,
// /
			0b0000000,
			0b0000000,
			0b0000001,
			0b0000010,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0100000,
			0b0000000,
			0b0000000,

// 0
			0b0000000,
			0b0000000,
			0b0011100,
			0b0100010,
			0b0100110,
			0b0101010,
			0b0110010,
			0b0011100,
			0b0000000,
			0b0000000,
// 1
			0b0000000,
			0b0000000,
			0b0000100,
			0b0001100,
			0b0010100,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0000000,
			0b0000000,
// 2
			0b0000000,
			0b0000000,
			0b0011100,
			0b0100010,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0111110,
			0b0000000,
			0b0000000,
// 3
			0b0000000,
			0b0000000,
			0b0111100,
			0b0000010,
			0b0011100,
			0b0000010,
			0b0000010,
			0b0111100,
			0b0000000,
			0b0000000,
// 4
			0b0000000,
			0b0000000,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0111110,
			0b0001000,
			0b0001000,
			0b0000000,
			0b0000000,
// 5
			0b0000000,
			0b0000000,
			0b0111100,
			0b0100000,
			0b0111100,
			0b0000010,
			0b0000010,
			0b0111100,
			0b0000000,
			0b0000000,
// 6
			0b0000000,
			0b0000000,
			0b0011100,
			0b0100000,
			0b0111100,
			0b0100010,
			0b0100010,
			0b0011100,
			0b0000000,
			0b0000000,
// 7
			0b0000000,
			0b0000000,
			0b0111110,
			0b0000010,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0010000,
			0b0000000,
			0b0000000,
// 8
			0b0000000,
			0b0000000,
			0b0011100,
			0b0100010,
			0b0011100,
			0b0100010,
			0b0100010,
			0b0011100,
			0b0000000,
			0b0000000,
// 9
			0b0000000,
			0b0000000,
			0b0011100,
			0b0100010,
			0b0011110,
			0b0000010,
			0b0000010,
			0b0011100,
			0b0000000,
			0b0000000,
// :
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0001000,
			0b0000000,
			0b0001000,
			0b0000000,
			0b0000000,
			0b0000000,
// ;
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0001000,
			0b0000000,
			0b0001000,
			0b0010000,
			0b0000000,
			0b0000000,
// <
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0001000,
			0b0000100,
			0b0000000,
			0b0000000,
// =
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0111110,
			0b0000000,
			0b0111110,
			0b0000000,
			0b0000000,
			0b0000000,
// >
			0b0000000,
			0b0000000,
			0b0000000,
			0b0010000,
			0b0001000,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0000000,
			0b0000000,
// ?
			0b0000000,
			0b0111100,
			0b0000010,
			0b0000010,
			0b0001100,
			0b0001000,
			0b0000000,
			0b0001000,
			0b0000000,
			0b0000000,

// @
			0b0000000,
			0b0000000,
			0b0011110,
			0b0100000,
			0b0101110,
			0b0101110,
			0b0100010,
			0b0011100,
			0b0000000,
			0b0000000,
// A
			0b0000000,
			0b0000000,
			0b0001000,
			0b0010100,
			0b0100010,
			0b0111110,
			0b0100010,
			0b0100010,
			0b0000000,
			0b0000000,
// B
			0b0000000,
			0b0000000,
			0b0111100,
			0b0100010,
			0b0111100,
			0b0100010,
			0b0100010,
			0b0111100,
			0b0000000,
			0b0000000,
// C
			0b0000000,
			0b0000000,
			0b0011110,
			0b0100000,
			0b0100000,
			0b0100000,
			0b0100000,
			0b0011110,
			0b0000000,
			0b0000000,
// D
			0b0000000,
			0b0000000,
			0b0111100,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0111100,
			0b0000000,
			0b0000000,
// E
			0b0000000,
			0b0000000,
			0b0111110,
			0b0100000,
			0b0111100,
			0b0100000,
			0b0100000,
			0b0111110,
			0b0000000,
			0b0000000,
// F
			0b0000000,
			0b0000000,
			0b0111110,
			0b0100000,
			0b0111100,
			0b0100000,
			0b0100000,
			0b0100000,
			0b0000000,
			0b0000000,
// G
			0b0000000,
			0b0000000,
			0b0111100,
			0b0100000,
			0b0100000,
			0b0101110,
			0b0100010,
			0b0011100,
			0b0000000,
			0b0000000,
// H
			0b0000000,
			0b0000000,
			0b0100010,
			0b0100010,
			0b0111110,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0000000,
			0b0000000,
// I
			0b0000000,
			0b0000000,
			0b0001000,
			0b0001000,
			0b0001000,
			0b0001000,
			0b0001000,
			0b0001000,
			0b0000000,
			0b0000000,
// J
			0b0000000,
			0b0000000,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0100100,
			0b0011000,
			0b0000000,
			0b0000000,
// K
			0b0000000,
			0b0000000,
			0b0100010,
			0b0100100,
			0b0111000,
			0b0100100,
			0b0100010,
			0b0100010,
			0b0000000,
			0b0000000,
// L
			0b0000000,
			0b0000000,
			0b0100000,
			0b0100000,
			0b0100000,
			0b0100000,
			0b0100000,
			0b0111110,
			0b0000000,
			0b0000000,
// M
			0b0000000,
			0b0000000,
			0b0100010,
			0b0110110,
			0b0101010,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0000000,
			0b0000000,
// N
			0b0000000,
			0b0000000,
			0b0100010,
			0b0110010,
			0b0101010,
			0b0101010,
			0b0100110,
			0b0100010,
			0b0000000,
			0b0000000,
// O
			0b0000000,
			0b0000000,
			0b0011100,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0011100,
			0b0000000,
			0b0000000,
// P
			0b0000000,
			0b0000000,
			0b0111100,
			0b0100010,
			0b0111100,
			0b0100000,
			0b0100000,
			0b0100000,
			0b0000000,
			0b0000000,
// Q
			0b0000000,
			0b0000000,
			0b0011100,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0100100,
			0b0011010,
			0b0000000,
			0b0000000,
// R
			0b0000000,
			0b0000000,
			0b0111100,
			0b0100010,
			0b0111100,
			0b0101000,
			0b0100100,
			0b0100010,
			0b0000000,
			0b0000000,
// S
			0b0000000,
			0b0000000,
			0b0011110,
			0b0100000,
			0b0011100,
			0b0000010,
			0b0000010,
			0b0111100,
			0b0000000,
			0b0000000,
// T
			0b0000000,
			0b0000000,
			0b0111110,
			0b0001000,
			0b0001000,
			0b0001000,
			0b0001000,
			0b0001000,
			0b0000000,
			0b0000000,
// U
			0b0000000,
			0b0000000,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0011100,
			0b0000000,
			0b0000000,
// V
			0b0000000,
			0b0000000,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0010100,
			0b0001000,
			0b0000000,
			0b0000000,
// W
			0b0000000,
			0b0000000,
			0b0100010,
			0b0100010,
			0b0100010,
			0b0101010,
			0b0110110,
			0b0100010,
			0b0000000,
			0b0000000,
// X
			0b0000000,
			0b0000000,
			0b0100010,
			0b0010100,
			0b0001000,
			0b0010100,
			0b0100010,
			0b0100010,
			0b0000000,
			0b0000000,
// Y
			0b0000000,
			0b0000000,
			0b0100010,
			0b0100010,
			0b0010100,
			0b0001000,
			0b0001000,
			0b0001000,
			0b0000000,
			0b0000000,
// Z
			0b0000000,
			0b0000000,
			0b0111110,
			0b0000100,
			0b0001000,
			0b0010000,
			0b0100000,
			0b0111110,
			0b0000000,
			0b0000000,
// [
			0b0000000,
			0b0011100,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0010000,
			0b0011100,
			0b0000000,
// backslash
			0b0000000,
			0b0000000,
			0b1000000,
			0b0100000,
			0b0010000,
			0b0001000,
			0b0000100,
			0b0000010,
			0b0000000,
			0b0000000,

// ]
			0b0000000,
			0b0011100,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0000100,
			0b0011100,
			0b0000000,
// ^
			0,0,0,0,0,0,0,0,0,0,
// _
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b0000000,
			0b1111111,
			0b0000000,
// `
			0,0,0,0,0,0,0,0,0,0,

// INVALID CODEPOINT PLACEHOLDER GLYPH:
			0b1111111,
			0b1111111,
			0b1111111,
			0b1111111,
			0b1111111,
			0b1111111,
			0b1111111,
			0b1111111,
			0b1111111,
			0b1111111,
};

static constexpr char GLYPH_COUNT = sizeof(font)/font_height - 1;
static constexpr char FIRST_GLYPH_CODEPOINT = ' ';
static constexpr char LAST_GLYPH_CODEPOINT = FIRST_GLYPH_CODEPOINT + GLYPH_COUNT - 1; // The last one is not for any code point!
	//!! Only for a contiguous set of code points directly mapped to the set of glyphs!

static constexpr char FIRST_GLYPH_INDEX = 0;
static constexpr char LAST_GLYPH_INDEX = FIRST_GLYPH_INDEX + GLYPH_COUNT;
//static constexpr char INVALID_GLYPH_INDEX = LAST_GLYPH_INDEX;
static constexpr char INVALID_GLYPH_INDEX = '?' - FIRST_GLYPH_CODEPOINT; //!! Only for ASCII, and a contiguous set of code points directly mapped to the set of glyphs!

static inline constexpr unsigned short font_glyph_index(char ch)
{//!! Only for ASCII & a contiguous set of code points directly mapped to the set of glyphs!
	return ch >= FIRST_GLYPH_CODEPOINT && ch <= LAST_GLYPH_CODEPOINT
		? ch - FIRST_GLYPH_CODEPOINT : INVALID_GLYPH_INDEX;
}

#endif // _CNXV4IU5YTB9678DT5674BTN6893476T0576FYM4780NBU87Y_