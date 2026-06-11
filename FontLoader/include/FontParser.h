#ifndef FONTPARSER_H
#define FONTPARSER_H

#include <stdint.h>
#include <stdio.h>

// Only for font collections (support for these tbd)
typedef struct TTCHeader TTCHeader;

// For .ttf and .otf files, this table comes at the very start of the file
typedef struct OTFTableDirectory OTFTableDirectory;

typedef struct OTFTableHEAD OTFTableHEAD;

typedef struct OTFTableHHEA OTFTableHHEA;

typedef struct OTFTableMAXP OTFTableMAXP;

typedef struct OTFTableHMTX OTFTableHMTX;

typedef struct OTFTableCMAP OTFTableCMAP;

typedef struct OTFCMAPFormat4 OTFCMAPFormat4;


typedef struct OTFTableLOCA OTFTableLOCA;

typedef struct OTFTableGLYF OTFTableGLYF;

typedef struct OTFFontFile {
    OTFTableHEAD* head;
    OTFTableHHEA* hhea;
    OTFTableMAXP* maxp;
    OTFTableHMTX* hmtx;
    OTFTableCMAP* cmap;
    OTFCMAPFormat4* format4;
    OTFTableLOCA* loca;
    OTFTableGLYF* glyf;
} OTFFontFile;

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint24[3];
typedef uint32_t uint32;
typedef int32_t int32;
typedef int32_t Fixed;
typedef int16_t FWORD;
typedef uint16_t UFWORD;
typedef int16_t F2DOT14;
typedef int64_t LONGDATETIME;
typedef uint8_t Tag[4];
typedef uint8_t Offset8;
typedef uint16_t Offset16;
typedef uint8_t Offset24[3];
typedef uint32_t Offset32;
typedef uint32_t Version16Dot16;

struct TTCHeader {
    Tag ttcTag; // 4 byte id string
    uint16 majorVersion;
    uint16 minorVersion; 
    uint32 numFonts;
    Offset32* tableDirectoryOffsets; // The length of the array would be numFonts
    uint32 dsigTag; // Indicates whether there is DSIG table (0x44534947 for 'DSIG' and null if there is no signature)
    uint32 dsigLength; // Length in bytes of the DSIG table (null if doesn't exist)
    uint32 dsigOffset; // Offsets (in bytes) to the DSIG table from the start of the file (null if no signature)
};

typedef struct TableRecord {
    Tag tableTag; // table identifier
    uint32 checksum;
    Offset32 offset; // offset from the beginning of the file
    uint32 length; // the length of the table
} TableRecord;

struct OTFTableDirectory {
    uint32 sfntVersion; // 0x00010000 or 0x4F54544F
    uint16 numTables;
    uint16 searchRange; // Maximum power of 2 less than or equal to numTables, times 16 ((2**floor(log2(numTables))) * 16
    uint16 entrySelector; // Log2 of the maximum power of 2 less than or equal to numTables (log2(searchRange/16), which is equal to floor(log2(numTables))).
    uint16 rangeShift; // numTables times 16, minus searchRange ((numTables * 16) - searchRange).
    TableRecord* tableRecords; // Table records array—one for each top-level table in the font. Has a length equal to numTables
};

struct OTFTableHEAD {
    uint16 majorVersion;
    uint16 minorVersion;
    Fixed fontRevision;
    uint32 checksumAdjustment;
    uint32 magicNumber; // 0x5F0F3CF5
    uint16 flags;
    uint16 unitsPerEm;
    LONGDATETIME created;
    LONGDATETIME modified;
    int16 xMin;
    int16 yMin;
    int16 xMax;
    int16 yMax;
    uint16 macStyle;
    uint16 lowestRecPPEM; // Smallest readable size in pixels.
    int16 fontDirectionHint; // Depcrecated (should be set to 2)
    int16 indexToLocFormat;
    int16 glyphDataFormat;
};

struct OTFTableHHEA {
    uint16 majorVersion;
    uint16 minorVersion;
    FWORD ascender;
    FWORD descender;
    FWORD lineGap;
    UFWORD advanceWidthMax;
    FWORD minLeftSideBearing;
    FWORD minRightSideBearing;
    FWORD xMaxExtent;
    int16 caretSlopeRise;
    int16 caretSlopeRun;
    int16 caretOffset;
    int16 reserved[4];
    int16 metricDataFormat;
    uint16 numberOfHMetrics;
};

struct OTFTableMAXP {
    Version16Dot16 version; // 0x00005000 for version 0.5 and 0x00010000 for version 1.0
    uint16 numGlyphs;
    // Everything below only applies to version 1.0
    uint16 maxPoints;
    uint16 maxContours;
    uint16 maxCompositePoints;
    uint16 maxCompositeContours;
    uint16 maxZones;
    uint16 maxTwilightPoints;
    uint16 maxStorage;
    uint16 maxFunctionDefs;
    uint16 maxInstructionDefs;
    uint16 maxStackElements;
    uint16 maxSizeOfInstructions;
    uint16 maxComponentElements;
    uint16 maxComponentDepth;
};

typedef struct LongHorMetric {
    UFWORD advanceWidth;
    FWORD lsb;
} LongHorMetric;

struct OTFTableHMTX {
    LongHorMetric* hMetrics;
    FWORD* leftSideBearings;
};

typedef struct EncodingRecord {
    uint16 platformID;
    uint16 encodingID;
    Offset32 subtableOffset;
} EncodingRecord;

struct OTFTableCMAP {
    uint16 version;
    uint16 numTables;
    EncodingRecord* encodingRecords;
};

// This works for platformID=3,encodingID=1 and platformID=0,encodingID=3 (this corresponds to Unicode BMP)
struct OTFCMAPFormat4 {
    uint16 format;
    uint16 length;
    uint16 language;
    uint16 segCountX2;
    uint16 searchRange;
    uint16 entrySelector;
    uint16 rangeShift;
    uint16* endCode; // has size of segCount (segCountX2 / 2)
    uint16 reservePad;
    uint16* startCode; // hase size of segCount
    int16* idDelta; // has size of segCount
    uint16* idRangeOffset; // hase size of segCount
    uint16* glyphIdArray; // variable size
    uint8_t* arena;
};

struct OTFTableLOCA {
    union {
        Offset16* offsets16;
        Offset32* offsets32;
    };
};

typedef struct GlyphHeader {
    int16 numberOfContours; // greater than or equal to zero --> simple glyph, negative --> composite glyph
    int16 xMin;
    int16 yMin;
    int16 xMax;
    int16 yMax;
} GlyphHeader;

#define SG_ON_CURVE_POINT 0x01
#define SG_X_SHORT_VECTOR 0x02
#define SG_Y_SHORT_VECTOR 0x04
#define SG_REPEAT_FLAG 0x08
#define SG_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR 0x10
#define SG_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR 0x20
#define SG_OVERLAP_SIMPLE 0x40

typedef struct SimpleGlyph {
    uint16* endPtsOfContours; // size equal to numberOfContours
    uint16 instructionLength;
    uint8* instructions; // has size equal to instructionLength
    // the flags, xCoordinates, and yCoordinates arrays all have the same size
    // each point has an associated xCoordinate, yCoordinate, and flag
    uint8* flags; 
    int16* xCoordinates;
    int16* yCoordinates;
    uint32 numPoints; // This is not included in font files, but I have added it for ease of use
} SimpleGlyph;

#define CG_ARG_1_AND_2_ARE_WORDS 0x0001
#define CG_ARGS_ARE_XY_VALUES 0x0002
#define CG_ROUND_XY_TO_GRID 0x0004
#define CG_WE_HAVE_A_SCALE 0x0008
#define CG_MORE_COMPONENTS 0x0020
#define CG_WE_HAVE_AN_X_AND_Y_SCALE 0x0040
#define CG_WE_HAVE_A_TWO_BY_TWO 0x0080
#define CG_WE_HAVE_INSTRUCTIONS 0x0100
#define CG_USE_MY_METRICS 0x0200
#define CG_OVERLAP_COMPOUND 0x0400
#define CG_SCALED_COMPONENT_OFFSET 0x0800
#define UNSCALED_COMPONENT_OFFSET 0x1000

typedef struct ComponentGlyphRecord {
    uint16 flags;
    uint16 glyphIndex;
    uint16 argument1;
    uint16 argument2;

    // Optional transform data
    F2DOT14 scale;
    F2DOT14 xscale;
    F2DOT14 yscale;
    F2DOT14 scale01;
    F2DOT14 scale10;
} ComponentGlyphRecord;

typedef struct CompositeGlyph {
    ComponentGlyphRecord* componentGlyphs;
    uint32 componentGlyphsLength;

    // Insturctions, if any exist, would come immediately after the last component glyph record
    uint16 numInstructions;
    uint8* instructions;
} CompositeGlyph;

typedef struct Glyph {
    GlyphHeader header;
    union {
        SimpleGlyph sg;
        CompositeGlyph cg;
    };
} Glyph;

struct OTFTableGLYF {
    Glyph* glyphs;
    uint32 numGlyphs;
};


// Note: I say ttf_file a lot as a parameter, but it could actually be either a .ttf or a .otf file

OTFTableDirectory* FontParser_acquire_table_directory(uint8_t* ttf_file);

void FontParser_release_table_directory(OTFTableDirectory** tableDir);

void FontParser_print_table_directory(OTFTableDirectory* tableDir, FILE* output);

OTFTableHEAD* FontParser_acquire_table_head(uint8_t* ttf_file, OTFTableDirectory* tableDir);

void FontParser_release_table_head(OTFTableHEAD** tableHead);

void FontParser_print_table_head(OTFTableHEAD* tableHead, FILE* output);

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

OTFCMAPFormat4* FontParser_acquire_cmap_format4(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableCMAP* tableCMAP);

void FontParser_release_cmap_format4(OTFCMAPFormat4** cmap_format4);

void FontParser_print_cmap_format4(OTFCMAPFormat4* cmap_format4, FILE* output);

OTFTableLOCA* FontParser_acquire_table_loca(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableHEAD* tableHEAD, OTFTableMAXP* tableMAXP);

void FontParser_release_table_loca(OTFTableLOCA** tableLOCA);

void FontParser_print_table_loca(OTFTableLOCA* tableLOCA, OTFTableHEAD* tableHEAD, OTFTableMAXP* tableMAXP, FILE* output);

OTFTableGLYF* FontParser_acquire_table_glyf(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableMAXP* tableMAXP, OTFTableLOCA* tableLOCA, OTFTableHEAD* tableHEAD);

void FontParser_release_table_glyf(OTFTableGLYF** tableGLYF);

void FontParser_print_table_glyf(OTFTableGLYF* tableGLYF, FILE* output);

OTFFontFile* FontParser_acquire_font(const char* font_file);

void FontParser_release_font(OTFFontFile** font_file);

void FontParser_print_font(OTFFontFile* font_file, FILE* output);

uint32_t FontParser_get_glyphID(OTFFontFile* font_file, uint32 character);

#endif