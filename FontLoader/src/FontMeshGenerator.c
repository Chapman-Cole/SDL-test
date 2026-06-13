#include "FontMeshGenerator.h"
#include "stdlib.h"
#include "uchar.h"
#include "string.h"

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

    return fontMesh;
}

void FontGenerator_release_font_mesh(OTFFontMesh** fontMesh) {

}

OTFFontMesh* FontGenerator_acquire_char_mesh(OTFFontFile* font, uint32_t character) {

}

void FontGenerator_release_char_mesh(OTFFontMesh** fontMesh) {

}