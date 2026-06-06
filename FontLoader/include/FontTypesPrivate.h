#ifndef FONTTYPESPRIVATE_H
#define FONTTYPESPRIVATE_H

#include "FontParser.h"

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

struct OTFTableHead {
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


#endif