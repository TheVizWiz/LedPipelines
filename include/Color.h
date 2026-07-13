#pragma once

// LedPipelines' own color types, RGBA and HSV. This is what lets the library stand alone without depending on FastLED:
// the color type is a plain value type (RGB bytes + an alpha/opacity byte + color math) with no hardware ties, so the
// core compiles against no external library. A backend (see LedOutput / outputs/FastLEDOutput.h) converts these colors
// to whatever the LED driver wants at the output boundary.
//
// Why RGBA (not CRGB): the fourth channel `a` is the pixel's opacity, folded directly into the color instead of a
// parallel opacity array - so a pixel is one 4-byte value (color + alpha) rather than a color plus a separate byte
// tracked in lockstep. The names RGBA / HSV are also deliberately DISTINCT from FastLED's CRGB / CHSV, so the FastLED
// backend can include both FastLED and this header in one translation unit without a redefinition collision.


#include <cstdint>

struct RGBA;

/**
 * A floating-point HSV color. Unlike FastLED's uint8_t CHSV (hue 0-255, artistic rainbow map), this keeps full
 * precision so hues can be interpolated smoothly and wrap cleanly. hue is in degrees and may be any value - it is
 * wrapped into [0, 360) at conversion time (e.g. 500 wraps to 140, -30 wraps to 330). s and v are in [0, 1].
 */
struct FHSV {
	float h = 0;
	float s = 1;
	float v = 1;

	FHSV() = default;

	FHSV(float h, float s, float v) : h(h), s(s), v(v) {}

	RGBA toRGBA();
};

struct RGBA {
	// The RGB channels, exposed under two names (r/g/b and red/green/blue) via a union, plus the alpha/opacity byte.
	// `a` is the pixel's opacity: 0 = fully transparent (unlit), 255 = fully opaque. It is NOT part of the RGB color
	// math below - compositing (TemporaryLedData) reads and writes it explicitly.
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};
		struct {
			uint8_t red;
			uint8_t green;
			uint8_t blue;
		};
		uint8_t raw[3];
	};
	uint8_t a;

	// The full FastLED HTML color table (all 148 CSS/X11 names + FastLED's FairyLight and TCL extensions), values
	// copied verbatim so RGBA::Green (0x008000), RGBA::Lime (0x00FF00), etc. match hardware exactly.
	typedef enum {
		AliceBlue = 0xF0F8FF,
		Amethyst = 0x9966CC,
		AntiqueWhite = 0xFAEBD7,
		Aqua = 0x00FFFF,
		Aquamarine = 0x7FFFD4,
		Azure = 0xF0FFFF,
		Beige = 0xF5F5DC,
		Bisque = 0xFFE4C4,
		Black = 0x000000,
		BlanchedAlmond = 0xFFEBCD,
		Blue = 0x0000FF,
		BlueViolet = 0x8A2BE2,
		Brown = 0xA52A2A,
		BurlyWood = 0xDEB887,
		CadetBlue = 0x5F9EA0,
		Chartreuse = 0x7FFF00,
		Chocolate = 0xD2691E,
		Coral = 0xFF7F50,
		CornflowerBlue = 0x6495ED,
		Cornsilk = 0xFFF8DC,
		Crimson = 0xDC143C,
		Cyan = 0x00FFFF,
		DarkBlue = 0x00008B,
		DarkCyan = 0x008B8B,
		DarkGoldenrod = 0xB8860B,
		DarkGray = 0xA9A9A9,
		DarkGrey = 0xA9A9A9,
		DarkGreen = 0x006400,
		DarkKhaki = 0xBDB76B,
		DarkMagenta = 0x8B008B,
		DarkOliveGreen = 0x556B2F,
		DarkOrange = 0xFF8C00,
		DarkOrchid = 0x9932CC,
		DarkRed = 0x8B0000,
		DarkSalmon = 0xE9967A,
		DarkSeaGreen = 0x8FBC8F,
		DarkSlateBlue = 0x483D8B,
		DarkSlateGray = 0x2F4F4F,
		DarkSlateGrey = 0x2F4F4F,
		DarkTurquoise = 0x00CED1,
		DarkViolet = 0x9400D3,
		DeepPink = 0xFF1493,
		DeepSkyBlue = 0x00BFFF,
		DimGray = 0x696969,
		DimGrey = 0x696969,
		DodgerBlue = 0x1E90FF,
		FireBrick = 0xB22222,
		FloralWhite = 0xFFFAF0,
		ForestGreen = 0x228B22,
		Fuchsia = 0xFF00FF,
		Gainsboro = 0xDCDCDC,
		GhostWhite = 0xF8F8FF,
		Gold = 0xFFD700,
		Goldenrod = 0xDAA520,
		Gray = 0x808080,
		Grey = 0x808080,
		Green = 0x008000,
		GreenYellow = 0xADFF2F,
		Honeydew = 0xF0FFF0,
		HotPink = 0xFF69B4,
		IndianRed = 0xCD5C5C,
		Indigo = 0x4B0082,
		Ivory = 0xFFFFF0,
		Khaki = 0xF0E68C,
		Lavender = 0xE6E6FA,
		LavenderBlush = 0xFFF0F5,
		LawnGreen = 0x7CFC00,
		LemonChiffon = 0xFFFACD,
		LightBlue = 0xADD8E6,
		LightCoral = 0xF08080,
		LightCyan = 0xE0FFFF,
		LightGoldenrodYellow = 0xFAFAD2,
		LightGreen = 0x90EE90,
		LightGrey = 0xD3D3D3,
		LightPink = 0xFFB6C1,
		LightSalmon = 0xFFA07A,
		LightSeaGreen = 0x20B2AA,
		LightSkyBlue = 0x87CEFA,
		LightSlateGray = 0x778899,
		LightSlateGrey = 0x778899,
		LightSteelBlue = 0xB0C4DE,
		LightYellow = 0xFFFFE0,
		Lime = 0x00FF00,
		LimeGreen = 0x32CD32,
		Linen = 0xFAF0E6,
		Magenta = 0xFF00FF,
		Maroon = 0x800000,
		MediumAquamarine = 0x66CDAA,
		MediumBlue = 0x0000CD,
		MediumOrchid = 0xBA55D3,
		MediumPurple = 0x9370DB,
		MediumSeaGreen = 0x3CB371,
		MediumSlateBlue = 0x7B68EE,
		MediumSpringGreen = 0x00FA9A,
		MediumTurquoise = 0x48D1CC,
		MediumVioletRed = 0xC71585,
		MidnightBlue = 0x191970,
		MintCream = 0xF5FFFA,
		MistyRose = 0xFFE4E1,
		Moccasin = 0xFFE4B5,
		NavajoWhite = 0xFFDEAD,
		Navy = 0x000080,
		OldLace = 0xFDF5E6,
		Olive = 0x808000,
		OliveDrab = 0x6B8E23,
		Orange = 0xFFA500,
		OrangeRed = 0xFF4500,
		Orchid = 0xDA70D6,
		PaleGoldenrod = 0xEEE8AA,
		PaleGreen = 0x98FB98,
		PaleTurquoise = 0xAFEEEE,
		PaleVioletRed = 0xDB7093,
		PapayaWhip = 0xFFEFD5,
		PeachPuff = 0xFFDAB9,
		Peru = 0xCD853F,
		Pink = 0xFFC0CB,
		Plaid = 0xCC5533,
		Plum = 0xDDA0DD,
		PowderBlue = 0xB0E0E6,
		Purple = 0x800080,
		Red = 0xFF0000,
		RosyBrown = 0xBC8F8F,
		RoyalBlue = 0x4169E1,
		SaddleBrown = 0x8B4513,
		Salmon = 0xFA8072,
		SandyBrown = 0xF4A460,
		SeaGreen = 0x2E8B57,
		Seashell = 0xFFF5EE,
		Sienna = 0xA0522D,
		Silver = 0xC0C0C0,
		SkyBlue = 0x87CEEB,
		SlateBlue = 0x6A5ACD,
		SlateGray = 0x708090,
		SlateGrey = 0x708090,
		Snow = 0xFFFAFA,
		SpringGreen = 0x00FF7F,
		SteelBlue = 0x4682B4,
		Tan = 0xD2B48C,
		Teal = 0x008080,
		Thistle = 0xD8BFD8,
		Tomato = 0xFF6347,
		Turquoise = 0x40E0D0,
		Violet = 0xEE82EE,
		Wheat = 0xF5DEB3,
		White = 0xFFFFFF,
		WhiteSmoke = 0xF5F5F5,
		Yellow = 0xFFFF00,
		YellowGreen = 0x9ACD32,

		FairyLight = 0xFFE42D,
		FairyLightNCC = 0xFF9D2A,

		Gray0 = 0x000000,
		Gray10 = 0x1A1A1A,
		Gray25 = 0x404040,
		Gray50 = 0x7F7F7F,
		Gray75 = 0xBFBFBF,
		Gray100 = 0xFFFFFF,
		Grey0 = 0x000000,
		Grey10 = 0x1A1A1A,
		Grey25 = 0x404040,
		Grey50 = 0x7F7F7F,
		Grey75 = 0xBFBFBF,
		Grey100 = 0xFFFFFF,

		Red1 = 0xFF0000,
		Red2 = 0xEE0000,
		Red3 = 0xCD0000,
		Red4 = 0x8B0000,
		Green1 = 0x00FF00,
		Green2 = 0x00EE00,
		Green3 = 0x00CD00,
		Green4 = 0x008B00,
		Blue1 = 0x0000FF,
		Blue2 = 0x0000EE,
		Blue3 = 0x0000CD,
		Blue4 = 0x00008B,
		Orange1 = 0xFFA500,
		Orange2 = 0xEE9A00,
		Orange3 = 0xCD8500,
		Orange4 = 0x8B5A00,
		Yellow1 = 0xFFFF00,
		Yellow2 = 0xEEEE00,
		Yellow3 = 0xCDCD00,
		Yellow4 = 0x8B8B00,
		Cyan1 = 0x00FFFF,
		Cyan2 = 0x00EEEE,
		Cyan3 = 0x00CDCD,
		Cyan4 = 0x008B8B,
		Magenta1 = 0xFF00FF,
		Magenta2 = 0xEE00EE,
		Magenta3 = 0xCD00CD,
		Magenta4 = 0x8B008B,
		VioletRed = 0xD02090,
		DeepPink1 = 0xFF1493,
		DeepPink2 = 0xEE1289,
		DeepPink3 = 0xCD1076,
		DeepPink4 = 0x8B0A50,
		Gold1 = 0xFFD700,
		Gold2 = 0xEEC900,
		Gold3 = 0xCDAD00,
		Gold4 = 0x8B7500,
	} HTMLColorCode;

	// Default is opaque black. RGB ctors default alpha to 255 (opaque) - a color literal like RGBA::Red is fully
	// opaque; compositing code overrides `a` when it wants a specific opacity.
	RGBA() : r(0), g(0), b(0), a(255) {}
	RGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) : r(red), g(green), b(blue), a(alpha) {}

	// 24-bit hex code (0xRRGGBB), including all the named-color enum values above. Alpha defaults to opaque.
	RGBA(uint32_t code) : r((code >> 16) & 0xFF), g((code >> 8) & 0xFF), b(code & 0xFF), a(255) {}

	RGBA(const RGBA& o) = default;
	RGBA& operator=(const RGBA& o) = default;

	RGBA& operator=(uint32_t code);

	RGBA operator*(RGBA second) const;

	RGBA operator*(float amount) const;

	RGBA& operator*=(float amount);

	// Per-channel saturating add (used by ADD blending). Alpha is left untouched - compositing sets it explicitly.
	RGBA operator+(RGBA second);

	// Per-channel multiply, scaled back into [0, 255] (used by MULTIPLY blending). Alpha untouched.
	RGBA& operator*=(RGBA second);

	// Value equality across the RGB channels AND alpha, so two pixels compare equal only if both color and opacity
	// match. Lets RGBA be compared directly (e.g. in tests via EXPECT_EQ).
	bool operator==(const RGBA& other) const {
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool operator!=(const RGBA& other) const {
		return !(*this == other);
	}
};
