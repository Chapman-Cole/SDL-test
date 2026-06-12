#include "FontGenerator.h"
#include "stdlib.h"
#include "uchar.h"
#include "string.h"

static inline int32_t f2dot14_mult(int32_t num, F2DOT14 frac) {
    return (num * frac + (1 << 13)) >> 14;
}

uint32_t glyph_get_num_points(OTFFontFile* font, Glyph* glyph) {
    if (glyph->header.numberOfContours > 0) {
        return glyph->sg.numPoints;
    } else if (glyph->header.numberOfContours < 0) {
        uint32_t numPoints = 0;
        for (uint32_t i = 0; i < glyph->cg.componentGlyphsLength; i++) {
            numPoints += font->glyf->glyphs[glyph->cg.componentGlyphs[i].glyphIndex].sg.numPoints;
        }
        return numPoints;
    } else {
        return 0;
    }
}

uint32_t glyph_get_num_indices(OTFFontFile* font, Glyph* glyph) {
    uint32_t numIndices = 0;
    if (glyph->header.numberOfContours > 0) {
        uint16_t startContourIdx = 0;
        for (uint32_t i = 0; i < glyph->header.numberOfContours; i++) {
            // To get the number of points per contour, find the difference between
            // successive indices to account for the inclusion of the starting contour index
            uint32_t numContourPoints = glyph->sg.endPtsOfContours[i] - startContourIdx + 1;
            startContourIdx = glyph->sg.endPtsOfContours[i] + 1;

            // An n-gon will have n-2 triangles, and each triangle will have 3 associated points,
            // which is the basis for calculating the number of indices to add for this contour
            numIndices += 3 * (numContourPoints - 2);
        }
    } else if (glyph->header.numberOfContours < 0) {

    } else {
        return 0;
    }
}

// The vertices pointer you provide the address of will be advanced by the number of points in the glyph automatically for you
int32_t glyph_set_vertices(OTFFontFile* font, uint32_t glyphID, float** vertices, int32_t globalOffsetX, int32_t globalOffsetY) {
    Glyph glyph = font->glyf->glyphs[glyphID < font->glyf->numGlyphs ? glyphID : 0];

    if (glyph.header.numberOfContours > 0) {
        // Simple glyph case
        int32_t cumulativeX = 0;
        int32_t cumulativeY = 0;

        for (uint32_t i = 0; i < glyph.sg.numPoints; i++) {
            cumulativeX += glyph.sg.xCoordinates[i];
            cumulativeY += glyph.sg.yCoordinates[i];

            int32_t absoluteX = cumulativeX + globalOffsetX;
            int32_t absoluteY = cumulativeY + globalOffsetY;

            (*vertices)[0] = (float)absoluteX;
            (*vertices)[1] = (float)absoluteY;
            (*vertices)[2] = 0.0;
            *vertices += 3; // This will advance the original pointer
        }
    } else if (glyph.header.numberOfContours < 0) {
        for (uint32_t i = 0; i < glyph.cg.componentGlyphsLength; i++) {
            // 1 in F2DOT14 format is 1 << 14 since the most significant bit determines sign, and the next bit after that
            // is the whole number part of the fraction
            F2DOT14 scaleX = 1 << 14;
            F2DOT14 scaleY = 1 << 14;

            F2DOT14 scale01 = 0;
            F2DOT14 scale10 = 0;

            int16_t offsetX = 0;
            int16_t offsetY = 0;
            
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // Remember to implement pivot point calculation at some point (that is the else condition for this flag)
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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

            Glyph currGlyph = font->glyf->glyphs[glyph.cg.componentGlyphs[i].glyphIndex];

            int32_t cumulativeX = 0;
            int32_t cumulativeY = 0;

            if (glyph.cg.componentGlyphs[i].flags & CG_SCALED_COMPONENT_OFFSET) {
                for (uint32_t j = 0; j < currGlyph.sg.numPoints; j++) {
                    cumulativeX += currGlyph.sg.xCoordinates[j];
                    cumulativeY += currGlyph.sg.yCoordinates[j];

                    int32_t absoluteX = cumulativeX;
                    int32_t absoluteY = cumulativeY;

                    int32_t temp_offsetx = offsetX;
                    int32_t temp_offsety = offsetY;
                    offsetX = f2dot14_mult(temp_offsetx, scaleX) + f2dot14_mult(temp_offsety, scale10);
                    offsetY = f2dot14_mult(temp_offsetx, scale01) + f2dot14_mult(temp_offsety, scaleY);

                    int32_t tempx = absoluteX;
                    int32_t tempy = absoluteY;
                    absoluteX = f2dot14_mult(tempx, scaleX) + f2dot14_mult(tempy, scale10);
                    absoluteY = f2dot14_mult(tempx, scale01) + f2dot14_mult(tempy, scaleY);

                    absoluteX += offsetX + globalOffsetX;
                    absoluteY += offsetY + globalOffsetY;

                    (*vertices)[0] = (float)absoluteX;
                    (*vertices)[1] = (float)absoluteY;
                    (*vertices)[2] = 0.0;
                    *vertices += 3; // This will advance the original pointer
                }
            } else {
                for (uint32_t j = 0; j < currGlyph.sg.numPoints; j++) {
                    cumulativeX += currGlyph.sg.xCoordinates[j];
                    cumulativeY += currGlyph.sg.yCoordinates[j];

                    int32_t absoluteX = cumulativeX;
                    int32_t absoluteY = cumulativeY;

                    int32_t tempx = absoluteX;
                    int32_t tempy = absoluteY;
                    absoluteX = f2dot14_mult(tempx, scaleX) + f2dot14_mult(tempy, scale10);
                    absoluteY = f2dot14_mult(tempx, scale01) + f2dot14_mult(tempy, scaleY);

                    absoluteX += offsetX + globalOffsetX;
                    absoluteY += offsetY + globalOffsetY;

                    (*vertices)[0] = (float)absoluteX;
                    (*vertices)[1] = (float)absoluteY;
                    (*vertices)[2] = 0.0;
                    *vertices += 3; // This will advance the original pointer
                }
            }
        }
    }
    // Characters with no contours/points will simply advance the cursor, which is why there is no else statement here

    return font->hmtx->hMetrics[glyphID < font->hhea->numberOfHMetrics ? glyphID : 0].advanceWidth;
}

OTFFontMesh* FontGenerator_acquire_font_mesh(OTFFontFile* font, char* utf8_sequence) {
    OTFFontMesh* fontMesh = (OTFFontMesh*)malloc(sizeof(OTFFontMesh));

    mbstate_t charState;
    memset(&charState, 0, sizeof(mbstate_t));

    char32_t character;
    size_t mbResult;

    char* utf8 = utf8_sequence;
    // Adding the +1 will include the null terminator, which results in mbResult=0 and allows
    // the while loop to exit
    size_t remainingBytes = strlen(utf8) + 1;

    char32_t* utf32_sequence = (char32_t*)malloc(remainingBytes * sizeof(char32_t));
    size_t utf32_sequence_len = 0;

    while (remainingBytes > 0) {
        mbResult = mbrtoc32(&character, utf8, remainingBytes, &charState);

        if (mbResult == 0) {
            break;
        } else if (mbResult == (size_t)-3) {
            utf32_sequence[utf32_sequence_len] = character;
            utf32_sequence_len++;
        } else if (mbResult == (size_t)-2) {
            printf("Multibyte sequence unexpectedly ended.\n");
            free(fontMesh);
            free(utf32_sequence);
            return NULL;
        } else if (mbResult == (size_t)-1) {
            printf("Invalid character sequence detected.\n");
            free(fontMesh);
            free(utf32_sequence);
            return NULL;
        } else {
            utf32_sequence[utf32_sequence_len] = character;
            utf32_sequence_len++;

            utf8 += mbResult;
            remainingBytes -= mbResult;
        }
    }

    char32_t* temp_reallocator = (char32_t*)realloc(utf32_sequence, utf32_sequence_len * sizeof(char32_t));
    if (temp_reallocator == NULL) {
        free(utf32_sequence);
        free(fontMesh);
        return NULL;
    } else {
        utf32_sequence = temp_reallocator;
    }

    // Count the number of points
    uint32_t numPoints = 0;
    for (uint32_t i = 0; i < utf32_sequence_len; i++) {
        numPoints += glyph_get_num_points(font, &font->glyf->glyphs[FontParser_get_glyphID(font, utf32_sequence[i])]);
    }

    fontMesh->numVertices = numPoints;
    fontMesh->vertices = (float*)malloc(fontMesh->numVertices * sizeof(float));

    int32_t cursorX = 0;
    int32_t cursorY = 0;

    float* temp_vertex_ptr = fontMesh->vertices;
    for (uint32_t i = 0; i < utf32_sequence_len; i++) {
        glyph_set_vertices(font, FontParser_get_glyphID(font, utf32_sequence[i]), &temp_vertex_ptr, cursorX, cursorY);
    }

    return fontMesh;
}

void FontGenerator_release_font_mesh(OTFFontMesh** fontMesh) {

}