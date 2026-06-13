#include "FontCurves.h"
#include <stdbool.h>

typedef struct GlyphScaler {
    int16_t offsetX;
    int16_t offsetY;
    F2DOT14 xscale;
    F2DOT14 yscale;
    F2DOT14 scale01;
    F2DOT14 scale10;
    bool scaledComponentOffset;
} GlyphScaler;

static inline int32_t f2dot14_mult(int32_t num, F2DOT14 frac) {
    return (num * frac + (1 << 13)) >> 14;
}

static void font_contour_get_simple_glyph_absolutes(OTFFontFile* font, uint32_t glyphID, DynamicArray* points) {
    Glyph glyph = font->glyf->glyphs[glyphID];

    DynamicArray_create(points, sizeof(intvec2));

    int32_t cumulativeX = 0;
    int32_t cumulativeY = 0;

    for (uint32_t i = 0; i < glyph.sg.numPoints; i++) {
        cumulativeX += glyph.sg.xCoordinates[i];
        cumulativeY += glyph.sg.yCoordinates[i];

        DynamicArray_append(points, &(intvec2){cumulativeX, cumulativeY});
    }
}

static void font_contour_scale_simple_glyph_absolutes(DynamicArray* points, GlyphScaler* glyphScaler) {
    if (glyphScaler->scaledComponentOffset == true) {
        for (uint32_t i = 0; i < points->len; i++) {
            int32_t absoluteX = ((intvec2*)points->arr)[i].x;
            int32_t absoluteY = ((intvec2*)points->arr)[i].y;

            int32_t offsetX = glyphScaler->offsetX;
            int32_t offsetY = glyphScaler->offsetY;

            int32_t temp_offsetx = offsetX;
            int32_t temp_offsety = offsetY;
            offsetX = f2dot14_mult(temp_offsetx, glyphScaler->xscale) + f2dot14_mult(temp_offsety, glyphScaler->scale10);
            offsetY = f2dot14_mult(temp_offsetx, glyphScaler->scale01) + f2dot14_mult(temp_offsety, glyphScaler->yscale);

            int32_t tempx = absoluteX;
            int32_t tempy = absoluteY;
            absoluteX = f2dot14_mult(tempx, glyphScaler->xscale) + f2dot14_mult(tempy, glyphScaler->scale10);
            absoluteY = f2dot14_mult(tempx, glyphScaler->scale01) + f2dot14_mult(tempy, glyphScaler->yscale);

            absoluteX += offsetX;
            absoluteY += offsetY;

            ((intvec2*)points->arr)[i].x = absoluteX;
            ((intvec2*)points->arr)[i].y = absoluteY;
        }
    } else {
        for (uint32_t i = 0; i < points->len; i++) {
            int32_t absoluteX = ((intvec2*)points->arr)[i].x;
            int32_t absoluteY = ((intvec2*)points->arr)[i].y;

            int32_t offsetX = glyphScaler->offsetX;
            int32_t offsetY = glyphScaler->offsetY;

            int32_t tempx = absoluteX;
            int32_t tempy = absoluteY;
            absoluteX = f2dot14_mult(tempx, glyphScaler->xscale) + f2dot14_mult(tempy, glyphScaler->scale10);
            absoluteY = f2dot14_mult(tempx, glyphScaler->scale01) + f2dot14_mult(tempy, glyphScaler->yscale);

            absoluteX += offsetX;
            absoluteY += offsetY;

            ((intvec2*)points->arr)[i].x = absoluteX;
            ((intvec2*)points->arr)[i].y = absoluteY;
        }
    }
}

static void font_contour_add_simple_glyph_points(FontCharacter* fontChar, OTFFontFile* font, uint32_t glyphID, GlyphScaler* glyphScaler) {
    DynamicArray absolutePoints;
    font_contour_get_simple_glyph_absolutes(font, glyphID, &absolutePoints);

    if (glyphScaler != NULL) {
        font_contour_scale_simple_glyph_absolutes(&absolutePoints, glyphScaler);
    }

    Glyph glyph = font->glyf->glyphs[glyphID];

    uint32_t startIndex = 0;
    for (uint32_t i = 0; i < glyph.header.numberOfContours; i++) {
        FontContourPoints contourPoints;
        DynamicArray_create(&contourPoints.points, sizeof(fvec2));
        DynamicArray_create(&contourPoints.flags, sizeof(bool));
        
        uint32_t endIndex = glyph.sg.endPtsOfContours[i];
        for (uint32_t j = startIndex; j <= endIndex; j++) {
            uint32_t j2 = j + 1;
            if (j2 > endIndex) {
                j2 = startIndex;
            }

            bool p1OnCurve = glyph.sg.flags[j] & SG_ON_CURVE_POINT;
            bool p2OnCurve = glyph.sg.flags[j2] & SG_ON_CURVE_POINT;

            intvec2 absP1 = ((intvec2*)absolutePoints.arr)[j];
            intvec2 absP2 = ((intvec2*)absolutePoints.arr)[j2];

            // append p1
            DynamicArray_append(&contourPoints.points, &(fvec2){.x = (float)absP1.x, .y = (float)absP1.y});
            DynamicArray_append(&contourPoints.flags, &p1OnCurve);

            if (
                p1OnCurve == false &&
                p2OnCurve == false
            ) {
                // append the implied on curve midpoint between p1 and p2 since there are consecutive off curve points
                DynamicArray_append(
                    &contourPoints.points,
                    &(fvec2){
                        .x = ((float)absP1.x + (float)absP2.x) / 2.0,
                        .y = ((float)absP1.y + (float)absP2.y) / 2.0
                    }
                );
                DynamicArray_append(&contourPoints.flags, &p2OnCurve);
            }
        }

        DynamicArray_append(&fontChar->contourPoints, &contourPoints);
    }

    DynamicArray_destroy(&absolutePoints);
}

static void font_contour_add_composite_glyph_points(FontCharacter* fontChar, OTFFontFile* font, uint32_t glyphID) {
    Glyph glyph = font->glyf->glyphs[glyphID];

    for (uint32_t i = 0; i < glyph.cg.componentGlyphsLength; i++) {
        uint32_t currGlyphID = glyph.cg.componentGlyphs[i].glyphIndex;

        // 1 in F2DOT14 format is 1 << 14 since the most significant bit determines sign, and the next bit after that
            // is the whole number part of the fraction
        F2DOT14 scaleX = 1 << 14;
        F2DOT14 scaleY = 1 << 14;

        F2DOT14 scale01 = 0;
        F2DOT14 scale10 = 0;

        int16_t offsetX = 0;
        int16_t offsetY = 0;

        if (glyph.cg.componentGlyphs[i].flags & CG_ARGS_ARE_XY_VALUES) {
            offsetX = glyph.cg.componentGlyphs[i].argument1;
            offsetY = glyph.cg.componentGlyphs[i].argument2;
        }

        if (glyph.cg.componentGlyphs[i].flags & CG_WE_HAVE_A_SCALE) {
            scaleX = glyph.cg.componentGlyphs[i].scale;
            scaleY = glyph.cg.componentGlyphs[i].scale;
        } else if (glyph.cg.componentGlyphs[i].flags & CG_WE_HAVE_AN_X_AND_Y_SCALE) {
            scaleX = glyph.cg.componentGlyphs[i].xscale;
            scaleY = glyph.cg.componentGlyphs[i].yscale;
        } else if (glyph.cg.componentGlyphs[i].flags & CG_WE_HAVE_A_TWO_BY_TWO) {
            scaleX = glyph.cg.componentGlyphs[i].xscale;
            scaleY = glyph.cg.componentGlyphs[i].yscale;
            scale01 = glyph.cg.componentGlyphs[i].scale01;
            scale10 = glyph.cg.componentGlyphs[i].scale10;
        }

        GlyphScaler glyphScaler = {
            .offsetX = offsetX,
            .offsetY = offsetY,
            .xscale = scaleX,
            .yscale = scaleY,
            .scale01 = scale01,
            .scale10 = scale10,
            .scaledComponentOffset = glyph.cg.componentGlyphs[i].flags & CG_SCALED_COMPONENT_OFFSET
        };

        font_contour_add_simple_glyph_points(fontChar, font, currGlyphID, &glyphScaler);
    }
}

void FontCharacter_create(FontCharacter* fontChar, OTFFontFile* font, uint32_t character) {
    uint32_t glyphID = FontParser_get_glyphID(font, character);

    DynamicArray_create(&fontChar->contourPoints, sizeof(FontContourPoints));
    DynamicArray_create(&fontChar->contours, sizeof(FontContour));

    Glyph glyph = font->glyf->glyphs[glyphID];

    if (glyph.header.numberOfContours > 0) {
        font_contour_add_simple_glyph_points(fontChar, font, glyphID, NULL);
    } else if (glyph.header.numberOfContours < 0) {
        font_contour_add_composite_glyph_points(fontChar, font, glyphID);
    } else {
        return;
    }  
}

void FontCharacter_destroy(FontCharacter* fontChar) {
    for (uint32_t i = 0; i < fontChar->contourPoints.len; i++) {
        FontContourPoints contourPoints = ((FontContourPoints*)fontChar->contourPoints.arr)[i];
        DynamicArray_destroy(&contourPoints.points);
        DynamicArray_destroy(&contourPoints.flags);
    }
    DynamicArray_destroy(&fontChar->contourPoints);

    for (uint32_t i = 0; i < fontChar->contours.len; i++) {
        FontContour contour = ((FontContour*)fontChar->contours.arr)[i];
        DynamicArray_destroy(&contour.curves);
        DynamicArray_destroy(&contour.lines);
    }
    DynamicArray_destroy(&fontChar->contours);
}