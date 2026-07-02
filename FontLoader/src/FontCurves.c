#include "FontCurves.h"
#include <stdbool.h>
#include <math.h>

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
    uint32_t endIndex = 0;
    for (uint32_t i = 0; i < glyph.header.numberOfContours; i++) {
        FontContourPoints contourPoints;
        DynamicArray_create(&contourPoints.points, sizeof(fvec2));
        DynamicArray_create(&contourPoints.flags, sizeof(bool));
        
        endIndex = glyph.sg.endPtsOfContours[i];
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
                // (bool[])(true) is a trick to append a single element to an array of bools because dereferencing the passed pointer
                // will give the value of the first element, which is true
                DynamicArray_append(&contourPoints.flags, (bool[]){true});
            }
        }

        DynamicArray_append(&fontChar->contourPoints, &contourPoints);

        startIndex = endIndex + 1;
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

void straight_line_calc_intersections(StraightLine* line, float hLineYVal, DynamicArray* intersections) {
    if ((hLineYVal > line->p1.y && hLineYVal > line->p2.y) || (hLineYVal < line->p1.y && hLineYVal < line->p2.y)) {
        return;
    }

    float numerator = line->p2.y - line->p1.y;
    float denominator = line->p2.x - line->p1.x;

    // Horizontal line in this case would mean infinite intersections since y values above or below hLineYVal were already filtered out.
    // For now, I think the easiest way to handle this case is to just append the two endpoints of the line
    if (numerator == 0) {
        DynamicArray_append(intersections, &(CurveIntersection){.curveType = CURVE_INTERSECTION_LINE, .line = *line, .point = line->p1});
        DynamicArray_append(intersections, &(CurveIntersection){.curveType = CURVE_INTERSECTION_LINE, .line = *line, .point = line->p2});
        return;
    }

    // Vertical line so just append a point with the x value of the line and the y value of hLineYVal
    if (denominator == 0) {
        DynamicArray_append(intersections, &(CurveIntersection){.curveType = CURVE_INTERSECTION_LINE, .line = *line, .point = (fvec2){line->p1.x, hLineYVal}});
        return;
    }

    // This just comes from solving for x in point slope
    float inverseSlope = denominator / numerator;
    float xIntersection = line->p1.x + inverseSlope * (hLineYVal - line->p1.y);

    DynamicArray_append(intersections, &(CurveIntersection){.curveType = CURVE_INTERSECTION_LINE, .line = *line, .point = (fvec2){xIntersection, hLineYVal}});
}

static inline fvec2 calc_bezier_curve(BezierCurve* curve, float t) {
    float oneMinusT = 1.0 - t;
    float oneMinusTSquared = oneMinusT * oneMinusT;
    float tSquared = t * t;

    return (fvec2){
        .x = oneMinusTSquared * curve->p1.x + 2.0 * oneMinusT * t * curve->control.x + tSquared * curve->p2.x,
        .y = oneMinusTSquared * curve->p1.y + 2.0 * oneMinusT * t * curve->control.y + tSquared * curve->p2.y
    };
}

void bezier_curve_calc_intersections(BezierCurve* curve, float hLineYVal, DynamicArray* intersections) {
    float ay = curve->p1.y - 2.0 * curve->control.y + curve->p2.y;
    float by = -2.0 * curve->p1.y + 2.0 * curve->control.y;
    float cy = curve->p1.y;
    cy -= hLineYVal; // Shift the curve down by hLineYVal so the roots correspond to the intersections, if any

    float discriminantY = (by * by) - 4.0 * ay * cy;

    if (discriminantY > 0.0) {
        float radical = sqrtf(discriminantY);
        float t1 = (-by - radical) / (2.0 * ay);
        float t2 = (-by + radical) / (2.0 * ay);

        fvec2 intersection1, intersection2;

        // If either t value is less than 0 or greater than 1, that means it is an extraneous solution since
        // bezier curves are really only meant to be used for t values between 0 and 1 (inclusive)

        if (t1 >= 0.0 && t1 <= 1.0) {
            fvec2 intersection1;
            intersection1 = calc_bezier_curve(curve, t1);
            DynamicArray_append(intersections, &(CurveIntersection){.curveType = CURVE_INTERSECTION_BEZIER, .bezier = *curve, .point = intersection1});
        }

        if (t2 >= 0.0 && t2 <= 1.0) {
            fvec2 intersection2;
            intersection2 = calc_bezier_curve(curve, t2);
            DynamicArray_append(intersections, &(CurveIntersection){.curveType = CURVE_INTERSECTION_BEZIER, .bezier = *curve, .point = intersection2});
        }
    } else if (discriminantY == 0.0) {
        float t = -by / (2.0 * ay);

        if (t >= 0.0 && t <= 1.0) {
            fvec2 intersectionPoint = calc_bezier_curve(curve, t);
            DynamicArray_append(intersections, &(CurveIntersection){.curveType = CURVE_INTERSECTION_BEZIER, .bezier = *curve, .point = intersectionPoint});
        }
    }
}

void FontCharacter_calc_intersections(FontCharacter* fontChar, float hLineYVal, DynamicArray* intersections) {
    DynamicArray_create(intersections, sizeof(CurveIntersection));

    for (uint32_t i = 0; i < fontChar->contours.len; i++) {
        FontContour currContour = ((FontContour*)fontChar->contours.arr)[i];

        // Calculate straight line intersections
        for (uint32_t j = 0; j < currContour.lines.len; j++) {
            StraightLine currLine = ((StraightLine*)currContour.lines.arr)[j];
            straight_line_calc_intersections(&currLine, hLineYVal, intersections);
        }

        // Calculate bezier curve line intersections
        for (uint32_t j = 0; j < currContour.curves.len; j++) {
            BezierCurve currCurve = ((BezierCurve*)currContour.curves.arr)[j];
            bezier_curve_calc_intersections(&currCurve, hLineYVal, intersections);
        }
    }
}

void FontCharacter_create(FontCharacter* fontChar, OTFFontFile* font, uint32_t character) {
    uint32_t glyphID = FontParser_get_glyphID(font, character);
    fontChar->glyphID = glyphID;
    fontChar->font = font;

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

    for (uint32_t i = 0; i < fontChar->contourPoints.len; i++) {
        FontContourPoints contourPoints = ((FontContourPoints*)fontChar->contourPoints.arr)[i];
        FontContour contour;
        DynamicArray_create(&contour.lines, sizeof(StraightLine));
        DynamicArray_create(&contour.curves, sizeof(BezierCurve));

        for (uint32_t j = 0; j < contourPoints.points.len; j++) {
            bool p1OnCurve = ((bool*)contourPoints.flags.arr)[j];
            bool p2OnCurve;

            fvec2 p1 = ((fvec2*)contourPoints.points.arr)[j];
            fvec2 p2;
            if (j == contourPoints.points.len - 1) {
                p2 = ((fvec2*)contourPoints.points.arr)[0];
                p2OnCurve = ((bool*)contourPoints.flags.arr)[0];
            } else {
                p2 = ((fvec2*)contourPoints.points.arr)[j + 1];
                p2OnCurve = ((bool*)contourPoints.flags.arr)[j + 1];
            }

            if (p1OnCurve == true && p2OnCurve == true) {
                StraightLine currLine = {
                    .p1 = {p1.x, p1.y},
                    .p2 = {p2.x, p2.y}
                };
                DynamicArray_append(&contour.lines, &currLine);
            } else if (p1OnCurve == true && p2OnCurve == false) {
                fvec2 p3;

                if (j == contourPoints.points.len - 2) {
                    p3 = ((fvec2*)contourPoints.points.arr)[0];
                } else if (j == contourPoints.points.len - 1 ) {
                    p3 = ((fvec2*)contourPoints.points.arr)[1];
                } else {
                    p3 = ((fvec2*)contourPoints.points.arr)[j + 2];
                }

                BezierCurve currCurve = {
                    .p1 = {p1.x, p1.y},
                    .control = {p2.x, p2.y},
                    .p2 = {p3.x, p3.y}
                };
                DynamicArray_append(&contour.curves, &currCurve);
            }
        }

        DynamicArray_append(&fontChar->contours, &contour);
    }
}

void curve_intersection_dynamic_array_sort_x(DynamicArray* dyn_arr) {
    for (uint32_t i = 1; i < dyn_arr->len; i++) {
        CurveIntersection curr_val = ((CurveIntersection*)dyn_arr->arr)[i];
        int64_t j = i - 1;

        while (j >= 0 && ((CurveIntersection*)dyn_arr->arr)[j].point.x > curr_val.point.x) {
            ((CurveIntersection*)dyn_arr->arr)[j + 1] = ((CurveIntersection*)dyn_arr->arr)[j];
            j--;
        }

        ((CurveIntersection*)dyn_arr->arr)[j + 1] = curr_val;
    }
}

void FontCharacter_generate_mesh(FontCharacter* fontChar, DynamicArray* vertices, DynamicArray* indices, uint32_t resolution) {
    DynamicArray_create(vertices, sizeof(fvec3));
    DynamicArray_create(indices, sizeof(uint32_t));

    Glyph glyph = fontChar->font->glyf->glyphs[fontChar->glyphID];

    float delta = (float)(glyph.header.yMax - glyph.header.yMin) / (float)resolution;
    float startYVal = (float)glyph.header.yMin;
    float endYVal;

    DynamicArray prevIntersections, currIntersections;
    FontCharacter_calc_intersections(fontChar, startYVal, &prevIntersections);
    curve_intersection_dynamic_array_sort_x(&prevIntersections);
    
    uint32_t prevIntersectionsStartIndex, currIntersectionsStartIndex;
    for (uint32_t i = 0; i < prevIntersections.len; i++) {
        fvec2 currPoint = ((CurveIntersection*)prevIntersections.arr)[i].point;
        DynamicArray_append(vertices, &(fvec3){currPoint.x, currPoint.y, 0.0});
    }

    prevIntersectionsStartIndex = 0;
    currIntersectionsStartIndex = vertices->len;

    for (uint32_t i = 0; i < resolution; i++) {
        endYVal = startYVal + delta;
        FontCharacter_calc_intersections(fontChar, endYVal, &currIntersections);
        curve_intersection_dynamic_array_sort_x(&currIntersections);

        for (size_t j = 0; j < currIntersections.len; j++) {
            fvec2 currPoint = ((CurveIntersection*)currIntersections.arr)[j].point;
            DynamicArray_append(vertices, &(fvec3){currPoint.x, currPoint.y, 0.0});
        }

        // The basic idea is to construct quads (which then have to be broken down into two triangles)
        if (prevIntersections.len != 0 && prevIntersections.len == currIntersections.len) {
            // !!!!!!! Make sure the length of the intersection arrays is not 0 !!!!!!!!!!!!!!!!
            // if this is forgotten, then the prevIntersections.len - 1 will cause the unsigned number to wrap around to its
            // maximum value and cause all sorts of memory issues because the loop runs a ton of times. (This was an absolute nightmare to debug.)

            for (size_t j = 0; j < prevIntersections.len - 1; j++) {
                if ((j+1) % 2 == 0) {
                    // Means you are in one of the holes defined by the contour
                    continue;
                }

                // Counter clockwise winding order
                // Triangle 1
                DynamicArray_append(indices, (uint32_t[]){j + currIntersectionsStartIndex});
                DynamicArray_append(indices, (uint32_t[]){j + prevIntersectionsStartIndex});
                DynamicArray_append(indices, (uint32_t[]){j + 1 + prevIntersectionsStartIndex});

                // Triangle 2
                DynamicArray_append(indices, (uint32_t[]){j + 1 + prevIntersectionsStartIndex});
                DynamicArray_append(indices, (uint32_t[]){j + currIntersectionsStartIndex});
                DynamicArray_append(indices, (uint32_t[]){j + 1 + currIntersectionsStartIndex});
            }
        } // handle the other cases later on

        DynamicArray_destroy(&prevIntersections);
        prevIntersections = currIntersections;

        prevIntersectionsStartIndex = currIntersectionsStartIndex;
        currIntersectionsStartIndex = vertices->len;

        startYVal = endYVal;
    }
    
    DynamicArray_destroy(&prevIntersections);
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