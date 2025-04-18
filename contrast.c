#include <math.h>

#include "contrast.h"


/* Linear RGB floating point color space for use in calculations  */
typedef struct {
	float r;
	float g;
	float b;
	float l;
} RGBf;



static float
sRGB_to_lin(const unsigned short c)
{
	/* Convert to normalized float. */
	const float f = c / 65535.0f;

	/* Convert from sRGB to linear space. */
	return (f <= 0.03928f) ? f / 12.92f : pow((f + 0.055f) / 1.055f, 2.4f);
}



static unsigned short
lin_to_sRGB(const float c)
{
	/* Convert to sRGB space. */
	const float f = (c <= 0.0031308f) ?
	                      12.92f * c : 1.055f * pow(c, 1.0f / 2.4f) - 0.055f;

	/* Clamp and convert back to 16-bit values. */
	return	(unsigned short)(fminf(fmaxf(f * 65535.0f, 0.0f), 65535.0f));
}



static RGBf
XftColor_to_RGBf( XftColor const * const c )
{
	const float r = sRGB_to_lin(c->color.red);
	const float g = sRGB_to_lin(c->color.green);
	const float b = sRGB_to_lin(c->color.blue);

	/* Calculate luminance. */
	const float l = 0.2126f * r + 0.7152f * g + 0.0722f * b;

	return (RGBf) {r, g, b, l};
}



static void
RGBf_to_XftColor( const RGBf rgb, XftColor * const c)
{
	c->color.red   = lin_to_sRGB(rgb.r);
	c->color.green = lin_to_sRGB(rgb.g);
	c->color.blue  = lin_to_sRGB(rgb.b);
}



static float
get_luminance(XftColor const * const c)
{
	const RGBf rgb = XftColor_to_RGBf(c);
	return rgb.l;
}



static float
get_contrast_ratio(const float fg_lum, const float bg_lum)
{
	if (fg_lum > bg_lum) {
		return (fg_lum + 0.05f) / (bg_lum + 0.05f);
	} else {
		return (bg_lum + 0.05f) / (fg_lum + 0.05f);
	}
}



static void
adjust_luminance(XftColor * const c, const float adjustment)
{
	/* Convert sRGB to linear space and calculate luminance. */
	RGBf rgb = XftColor_to_RGBf(c);

	/* Adjust luminance, clamping to 0-100% */
	const float factor = fminf(fmaxf(rgb.l + adjustment, 0.0f), 1.0f) / rgb.l;
	rgb.r *= factor;
	rgb.g *= factor;
	rgb.b *= factor;

	/* Convert back to sRGB space. */
	RGBf_to_XftColor(rgb, c);
}


static float
get_luminance_adjustment( float fl, float bl, float contrast )
{
	/* Increase existing any luminance difference to get contrast. */
	float adjustment = (bl > fl)?
	          ((((bl + 0.05f) / min_contrast_ratio) - 0.05f) - fl) :
	          (((min_contrast_ratio * (bl + 0.05f)) - 0.05f) - fl);

	const float new_lum = fl + adjustment;

	/* Use the opposite direction if we exceed valid luminance range. */
	if (new_lum < 0.0 || new_lum > 1.0) {
		adjustment = (bl <= fl)?
		    ((((bl + 0.05f) / min_contrast_ratio) - 0.05f) - fl) :
		    (((min_contrast_ratio * (bl + 0.05f)) - 0.05f) - fl);
	}

	return adjustment;
}


void
adjust_color_for_contrast(XftColor * const fg, XftColor * const bg)
{
	float fl = get_luminance(fg);
	const float bl = get_luminance(bg);
	const float contrast = get_contrast_ratio(fl, bl);

	if (contrast < min_contrast_ratio) {
		/* Change black to dark grey so the luminance calculations can work. */
		if (fl < 0.00001) {
			fg->color.red   = 0x1fff;
			fg->color.green = 0x1fff;
			fg->color.blue  = 0x1fff;
			fl = get_luminance(fg);
		}

		const float adjustment = get_luminance_adjustment(
		                             fl, bl, min_contrast_ratio);

		adjust_luminance(fg, adjustment);
	}
}
