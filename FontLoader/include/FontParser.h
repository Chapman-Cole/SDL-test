#ifndef FONTPARSER_H
#define FONTPARSER_H

#include <stdint.h>

// Only for font collections (support for these tbd)
typedef struct TTCHeader TTCHeader;

// For .ttf and .otf files, this table comes at the very start of the file
typedef struct OTFTableDirectory OTFTableDirectory;

typedef struct OTFTableHead OTFTableHead;

typedef struct OTFTableHHEA OTFTableHHEA;


OTFTableDirectory* FontParser_acquire_table_directory(uint8_t* ttf_file);

void FontParser_release_table_directory(OTFTableDirectory** tableDir);

void FontParser_print_table_directory(OTFTableDirectory* tableDir);

OTFTableHead* FontParser_acquire_table_head(uint8_t* ttf_file, OTFTableDirectory* tableDir);

void FontParser_release_table_head(OTFTableHead** tableHead);

void FontParser_print_table_head(OTFTableHead* tableHead);

OTFTableHHEA* FontParser_acquire_table_hhea(uint8_t* ttf_file, OTFTableDirectory* tableDir);

void FontParser_release_table_hhea(OTFTableHHEA** tableHHEA);

void FontParser_print_table_hhea(OTFTableHHEA* tableHHEA);

#endif