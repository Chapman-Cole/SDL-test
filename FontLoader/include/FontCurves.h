#ifndef FONTCURVES_H
#define FONTCURVES_H

#include "FontVectorTypes.h"
#include "FontParser.h"

typedef struct StraightLine {
    fvec2 p1;
    fvec2 p2;
} StraightLine;

typedef struct BezierCurve {
    fvec2 p1;
    fvec2 control;
    fvec2 p2;
} BezierCurve;

typedef struct FontContour {
    DynamicArray lines; // Dynamic array of StraightLine
    DynamicArray curves; // Dynamic array of BezierCurve
} FontContour;

typedef struct FontContourPoints {
    DynamicArray points; // A dynamic array of fvec2
    DynamicArray flags; // A dynamic array of bools
} FontContourPoints;

typedef struct FontCharacter {
    DynamicArray contours; // A dynamic array of FontContours
    DynamicArray contourPoints; // A dynamic array of FontContourPoints
    OTFFontFile* font;
    uint32_t glyphID;
} FontCharacter;

void FontCharacter_create(FontCharacter* fontChar, OTFFontFile* font, uint32_t character);

void FontCharacter_destroy(FontCharacter* fontChar);

// intersections will become a dynamic array of fvec2 type
void FontCharacter_calc_intersections(FontCharacter* fontChar, float hLineYVal, DynamicArray* intersections);

// vertices will become a dynamic array of fvec3s (which can also just be treated as an array of floats) and indices 
// will become a dynamic array of uint32_ts
void FontCharacter_generate_mesh(FontCharacter* fontChar, DynamicArray* vertices, DynamicArray* indices, uint32_t resolution);


#endif