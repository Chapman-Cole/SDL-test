#ifndef FONTPARSER_H
#define FONTPARSER_H

#include <stdint.h>
#include <stdio.h>

// Only for font collections (support for these tbd)
typedef struct TTCHeader TTCHeader;

// For .ttf and .otf files, this table comes at the very start of the file
typedef struct OTFTableDirectory OTFTableDirectory;

typedef struct OTFTableHead OTFTableHead;

typedef struct OTFTableHHEA OTFTableHHEA;

typedef struct OTFTableMAXP OTFTableMAXP;

typedef struct OTFTableHMTX OTFTableHMTX;

typedef struct OTFTableCMAP OTFTableCMAP;

// Note: I say ttf_file a lot as a parameter, but it could actually be either a .ttf or a .otf file

OTFTableDirectory* FontParser_acquire_table_directory(uint8_t* ttf_file);

void FontParser_release_table_directory(OTFTableDirectory** tableDir);

void FontParser_print_table_directory(OTFTableDirectory* tableDir, FILE* output);

OTFTableHead* FontParser_acquire_table_head(uint8_t* ttf_file, OTFTableDirectory* tableDir);

void FontParser_release_table_head(OTFTableHead** tableHead);

void FontParser_print_table_head(OTFTableHead* tableHead, FILE* output);

OTFTableHHEA* FontParser_acquire_table_hhea(uint8_t* ttf_file, OTFTableDirectory* tableDir);

void FontParser_release_table_hhea(OTFTableHHEA** tableHHEA);

void FontParser_print_table_hhea(OTFTableHHEA* tableHHEA, FILE* output);

OTFTableMAXP* FontParser_acquire_table_maxp(uint8_t* ttf_file, OTFTableDirectory* tableDir);

void FontParser_release_table_maxp(OTFTableMAXP** tableMAXP);

void FontParser_print_table_maxp(OTFTableMAXP* tableMAXP, FILE* output);

OTFTableHMTX* FontParser_acquire_table_hmtx(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableHHEA* tableHHEA, OTFTableMAXP* tableMAXP);

void FontParser_release_table_hmtx(OTFTableHMTX** tableHMTX);

void FontParser_print_table_hmtx(OTFTableHMTX* tableHMTX, OTFTableHHEA* tableHHEA, OTFTableMAXP* tableMAXP, FILE* output);

OTFTableCMAP* FontParser_acquire_table_cmap(uint8_t* ttf_file, OTFTableDirectory* tableDir);

void FontParser_release_table_cmap(OTFTableCMAP** tableCMAP);

void FontParser_print_table_cmap(OTFTableCMAP* tableCMAP, FILE* output);



#endif