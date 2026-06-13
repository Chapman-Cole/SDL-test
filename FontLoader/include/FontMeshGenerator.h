#ifndef FONTMESHGENERATOR_H
#define FONTMESHGENERATOR_H

#include "FontParser.h"

typedef struct OTFFontMesh {
    uint32_t numVertices;
    float* vertices;

    uint32_t numIndices;
    uint32_t* indices;
} OTFFontMesh;

OTFFontMesh* FontGenerator_acquire_font_mesh(OTFFontFile* font, char* utf8_sequence);

void FontGenerator_release_font_mesh(OTFFontMesh** fontMesh);

OTFFontMesh* FontGenerator_acquire_char_mesh(OTFFontFile* font, uint32_t character);

void FontGenerator_release_char_mesh(OTFFontMesh** fontMesh);

#endif