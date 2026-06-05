#include "FontParser.h"
#include "ParsingHelpers.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Many of the comments and typedef names come from the microsoft documentation: https://learn.microsoft.com/en-us/typography/opentype/spec/otff

#define OTF_EPOCH_OFFSET 2082844800LL

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

OTFTableDirectory* FontParser_acquire_table_directory(uint8_t* ttf_file) {
    OTFTableDirectory* tableDirectory = (OTFTableDirectory*)malloc(sizeof(OTFTableDirectory));
    if (tableDirectory == NULL) {
        printf("Failed to allocate memory for table directory\n");
        exit(-1);
    }

    tableDirectory->sfntVersion = read_32b(ttf_file);
    tableDirectory->numTables = read_16b(ttf_file + 4);
    tableDirectory->searchRange = read_16b(ttf_file + 6);
    tableDirectory->entrySelector = read_16b(ttf_file + 8);
    tableDirectory->rangeShift = read_16b(ttf_file + 10);

    tableDirectory->tableRecords = (TableRecord*)malloc(tableDirectory->numTables * sizeof(TableRecord));
    if (tableDirectory->tableRecords == NULL) {
        printf("Failed to allocate memory for table records\n");
        exit(-1);
    }

    // The sum of the previous offsets was 12 bytes
    uint32_t offsetCounter = 12;

    for (int i = 0; i < tableDirectory->numTables; i++) {
        TableRecord* tr = &tableDirectory->tableRecords[i];

        tr->tableTag[0] = ttf_file[offsetCounter];
        tr->tableTag[1] = ttf_file[offsetCounter + 1];
        tr->tableTag[2] = ttf_file[offsetCounter + 2];
        tr->tableTag[3] = ttf_file[offsetCounter + 3];

        tr->checksum = read_32b(ttf_file + offsetCounter + 4);
        tr->offset = read_32b(ttf_file + offsetCounter + 8);
        tr->length = read_32b(ttf_file + offsetCounter + 12);

        offsetCounter += 16;
    }

    return tableDirectory;
}

void FontParser_release_table_directory(OTFTableDirectory** tableDir) {
    free((*tableDir)->tableRecords);
    free(*tableDir);
    *tableDir = NULL;
}

void FontParser_print_table_directory(OTFTableDirectory* tableDir) {
    printf(
        "Table Directory: {\n"
        "   sfntVersion: 0x%X\n"
        "   numTables: %u\n"
        "   searchRange: %u\n"
        "   entrySelector: %u\n"
        "   rangeShift: %u\n"
        "   tableRecords: {\n",
        tableDir->sfntVersion,
        tableDir->numTables,
        tableDir->searchRange,
        tableDir->entrySelector,
        tableDir->rangeShift
    );

    for (int i = 0; i < tableDir->numTables; i++) {
        TableRecord tr = tableDir->tableRecords[i];

        printf(
            "   {\n"
            "       tableTag: %.*s\n"
            "       checksum: %u\n"
            "       offset: %u\n"
            "       length: %u\n"
            "   }\n\n",
            4,
            tr.tableTag,
            tr.checksum,
            tr.offset,
            tr.length
        );
    }

    printf("   }\n}\n");
}

TableRecord* get_table_record(OTFTableDirectory* tableDir, Tag tableTag) {
    for (int i = 0; i < tableDir->numTables; i++) {
        TableRecord* tr = &tableDir->tableRecords[i];

        if (
            tr->tableTag[0] == tableTag[0] &&
            tr->tableTag[1] == tableTag[1] &&
            tr->tableTag[2] == tableTag[2] &&
            tr->tableTag[3] == tableTag[3]
        ) {
            return tr;
        }
    }

    return NULL;
}

OTFTableHead* FontParser_acquire_table_head(uint8_t* ttf_file, OTFTableDirectory* tableDir) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'h', 'e', 'a', 'd'});

    uint32 head_offset = tableRec->offset;
    uint32 head_length = tableRec->length;

    OTFTableHead* tableHead = (OTFTableHead*)malloc(sizeof(OTFTableHead));

    uint8_t* header = ttf_file + head_offset;

    tableHead->majorVersion = advance_16b(&header);
    tableHead->minorVersion = advance_16b(&header);
    tableHead->fontRevision = advance_32b(&header);
    tableHead->checksumAdjustment = advance_32b(&header);
    tableHead->magicNumber = advance_32b(&header);
    tableHead->flags = advance_16b(&header);
    tableHead->unitsPerEm = advance_16b(&header);
    tableHead->created = advance_64b(&header);
    tableHead->modified = advance_64b(&header);
    tableHead->xMin = advance_16b(&header);
    tableHead->yMin = advance_16b(&header);
    tableHead->xMax = advance_16b(&header);
    tableHead->yMax = advance_16b(&header);
    tableHead->macStyle = advance_16b(&header);
    tableHead->lowestRecPPEM = advance_16b(&header);
    tableHead->fontDirectionHint = advance_16b(&header);
    tableHead->indexToLocFormat = advance_16b(&header);
    tableHead->glyphDataFormat = advance_16b(&header);

    return tableHead;
}

void FontParser_release_table_head(OTFTableHead** tableHead) {
    free(*tableHead);
    *tableHead = NULL;
}

void FontParser_print_table_head(OTFTableHead* tableHead) {
    char created_time[24];
    char modified_time[24];

    time_t time_of_creation = tableHead->created - OTF_EPOCH_OFFSET;
    time_t time_of_modification = tableHead->modified - OTF_EPOCH_OFFSET;

    struct tm GMTTime;
    #ifdef _WIN32
        gmtime_s(&GMTTime, &time_of_creation);
    #else
        gmtime_r(&time_of_creation, &GMTTime);
    #endif
    strftime(created_time, sizeof(created_time), "%m/%d/%y", &GMTTime);

    #ifdef _WIN32
        gmtime_s(&GMTTime, &time_of_modification);
    #else
        gmtime_r(&time_of_modification, &GMTTime);
    #endif
    strftime(modified_time, sizeof(modified_time), "%m/%d/%y", &GMTTime);

    printf(
        "Table Head: {\n"
        "   majorVersion: %u\n"
        "   minorVersion: %u\n"
        "   fontRevision: %u\n"
        "   checksumAdjustment: %u\n"
        "   magicNumber: 0x%X\n"
        "   flags: %u\n"
        "   unitsPerEm: %u\n"
        "   created: %s\n"
        "   modified: %s\n"
        "   xMin: %d\n"
        "   yMin: %d\n"
        "   xMax: %d\n"
        "   yMax: %d\n"
        "   macStyle: %u\n"
        "   lowestRecPPEM: %u\n"
        "   fontDirectionHint: %d\n"
        "   indexToLocFormat: %d\n"
        "   glyphDataFormat: %d\n"
        "}\n",
        tableHead->majorVersion,
        tableHead->minorVersion,
        tableHead->fontRevision,
        tableHead->checksumAdjustment,
        tableHead->magicNumber,
        tableHead->flags,
        tableHead->unitsPerEm,
        created_time,
        modified_time,
        tableHead->xMin,
        tableHead->yMin,
        tableHead->xMax,
        tableHead->yMax,
        tableHead->macStyle,
        tableHead->lowestRecPPEM,
        tableHead->fontDirectionHint,
        tableHead->indexToLocFormat,
        tableHead->glyphDataFormat
    );
}


OTFTableHHEA* FontParser_acquire_table_hhea(uint8_t* ttf_file, OTFTableDirectory* tableDir) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'h', 'h', 'e', 'a'});

    uint32 hhea_offset = tableRec->offset;
    uint32 hhea_length = tableRec->length;

    OTFTableHHEA* table_hhea = (OTFTableHHEA*)malloc(sizeof(OTFTableHHEA));

    uint8_t* hhea_ptr = ttf_file + hhea_offset;

    table_hhea->majorVersion = advance_16b(&hhea_ptr);
    table_hhea->minorVersion = advance_16b(&hhea_ptr);
    table_hhea->ascender = advance_16b(&hhea_ptr);
    table_hhea->descender = advance_16b(&hhea_ptr);
    table_hhea->lineGap = advance_16b(&hhea_ptr);
    table_hhea->advanceWidthMax = advance_16b(&hhea_ptr);
    table_hhea->minLeftSideBearing = advance_16b(&hhea_ptr);
    table_hhea->minRightSideBearing = advance_16b(&hhea_ptr);
    table_hhea->xMaxExtent = advance_16b(&hhea_ptr);
    table_hhea->caretSlopeRise = advance_16b(&hhea_ptr);
    table_hhea->caretSlopeRun = advance_16b(&hhea_ptr);
    table_hhea->caretOffset = advance_16b(&hhea_ptr);
    advance_64b(&hhea_ptr);
    table_hhea->metricDataFormat = advance_16b(&hhea_ptr);
    table_hhea->numberOfHMetrics = advance_16b(&hhea_ptr);

    return table_hhea;
}

void FontParser_release_table_hhea(OTFTableHHEA** tableHHEA) {
    free(*tableHHEA);
    *tableHHEA = NULL;
}

void FontParser_print_table_hhea(OTFTableHHEA* tableHHEA) {
    printf(
        "Table HHEA: {\n"
        "   majorVersion: %u\n"
        "   minorVersion: %u\n"
        "   ascender: %d\n"
        "   descender: %d\n"
        "   lineGap: %d\n"
        "   advanceWidthMax: %u\n"
        "   minLeftSideBearing: %d\n"
        "   minRightSideBearing: %d\n"
        "   xMaxExtent: %d\n"
        "   caretSlopeRise: %d\n"
        "   caretSlopeRun: %d\n"
        "   caretOffset: %d\n"
        "   metricDataFormat: %d\n"
        "   numberOfHMetrics: %u\n"
        "}\n",
        tableHHEA->majorVersion,
        tableHHEA->minorVersion,
        tableHHEA->ascender,
        tableHHEA->descender,
        tableHHEA->lineGap,
        tableHHEA->advanceWidthMax,
        tableHHEA->minLeftSideBearing,
        tableHHEA->minRightSideBearing,
        tableHHEA->xMaxExtent,
        tableHHEA->caretSlopeRise,
        tableHHEA->caretSlopeRun,
        tableHHEA->caretOffset,
        tableHHEA->metricDataFormat,
        tableHHEA->numberOfHMetrics
    );
}