#ifndef __ST_CONTRAST_H_
#define __ST_CONTRAST_H_

#include <X11/Xft/Xft.h>

void adjust_color_for_contrast(XftColor * const , XftColor * const );

/* config.h globals */
extern const float min_contrast_ratio;

#endif
