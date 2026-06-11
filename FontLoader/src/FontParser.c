#include "FontParser.h"
#include "ParsingHelpers.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

// Many of the comments and typedef names come from the microsoft documentation: https://learn.microsoft.com/en-us/typography/opentype/spec/otff

#define OTF_EPOCH_OFFSET 2082844800LL

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

void FontParser_print_table_directory(OTFTableDirectory* tableDir, FILE* output) {
    fprintf(
        output,
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

        fprintf(
            output,
            "       {\n"
            "           tableTag: %.*s\n"
            "           checksum: %u\n"
            "           offset: %u\n"
            "           length: %u\n"
            "       }\n",
            4,
            tr.tableTag,
            tr.checksum,
            tr.offset,
            tr.length
        );
    }

    fprintf(output, "   }\n}\n");
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

OTFTableHEAD* FontParser_acquire_table_head(uint8_t* ttf_file, OTFTableDirectory* tableDir) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'h', 'e', 'a', 'd'});

    uint32 head_offset = tableRec->offset;
    uint32 head_length = tableRec->length;

    OTFTableHEAD* tableHead = (OTFTableHEAD*)malloc(sizeof(OTFTableHEAD));

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

void FontParser_release_table_head(OTFTableHEAD** tableHead) {
    free(*tableHead);
    *tableHead = NULL;
}

void FontParser_print_table_head(OTFTableHEAD* tableHead, FILE* output) {
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

    fprintf(
        output,
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

void FontParser_print_table_hhea(OTFTableHHEA* tableHHEA, FILE* output) {
    fprintf(
        output,
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

OTFTableMAXP* FontParser_acquire_table_maxp(uint8_t* ttf_file, OTFTableDirectory* tableDir) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'m', 'a', 'x', 'p'});

    uint32 maxp_offset = tableRec->offset;
    uint32 maxp_length = tableRec->length;

    OTFTableMAXP* table_maxp = (OTFTableMAXP*)malloc(sizeof(OTFTableMAXP));
    memset(table_maxp, 0, sizeof(OTFTableMAXP));

    uint8_t* maxp_ptr = ttf_file + maxp_offset;

    table_maxp->version = advance_32b(&maxp_ptr);
    table_maxp->numGlyphs = advance_16b(&maxp_ptr);

    if (table_maxp->version == 0x00005000) {
        return table_maxp;
    }

    table_maxp->maxPoints = advance_16b(&maxp_ptr);
    table_maxp->maxContours = advance_16b(&maxp_ptr);
    table_maxp->maxCompositePoints = advance_16b(&maxp_ptr);
    table_maxp->maxCompositeContours = advance_16b(&maxp_ptr);
    table_maxp->maxZones = advance_16b(&maxp_ptr);
    table_maxp->maxTwilightPoints = advance_16b(&maxp_ptr);
    table_maxp->maxStorage = advance_16b(&maxp_ptr);
    table_maxp->maxFunctionDefs = advance_16b(&maxp_ptr);
    table_maxp->maxInstructionDefs = advance_16b(&maxp_ptr);
    table_maxp->maxStackElements = advance_16b(&maxp_ptr);
    table_maxp->maxSizeOfInstructions = advance_16b(&maxp_ptr);
    table_maxp->maxComponentElements = advance_16b(&maxp_ptr);
    table_maxp->maxComponentDepth = advance_16b(&maxp_ptr);

    return table_maxp;
}

void FontParser_release_table_maxp(OTFTableMAXP** tableMAXP) {
    free(*tableMAXP);
    *tableMAXP = NULL;
}

void FontParser_print_table_maxp(OTFTableMAXP* tableMAXP, FILE* output) {
    if (tableMAXP->version == 0x00005000) {
        fprintf(
            output,
            "Table MAXP {\n"
            "   version: %X\n"
            "   numGlyphs: %u\n"
            "}\n",
            tableMAXP->version,
            tableMAXP->numGlyphs
        );
    } else {
        fprintf(
            output,
            "Table MAXP {\n"
            "   version: 0x%X\n"
            "   numGlyphs: %u\n"
            "   maxPoints: %u\n"
            "   maxContours: %u\n"
            "   maxCompositePoints: %u\n"
            "   maxCompositeContours: %u\n"
            "   maxZones: %u\n"
            "   maxTwilightPoints: %u\n"
            "   maxStorage: %u\n"
            "   maxFunctionDefs: %u\n"
            "   maxInstructionDefs: %u\n"
            "   maxStackElements: %u\n"
            "   maxSizeOfInstructions: %u\n"
            "   maxComponentElements: %u\n"
            "   maxComponentDepth: %u\n"
            "}\n",
            tableMAXP->version,
            tableMAXP->numGlyphs,
            tableMAXP->maxPoints,
            tableMAXP->maxContours,
            tableMAXP->maxCompositePoints,
            tableMAXP->maxCompositeContours,
            tableMAXP->maxZones,
            tableMAXP->maxTwilightPoints,
            tableMAXP->maxStorage,
            tableMAXP->maxFunctionDefs,
            tableMAXP->maxInstructionDefs,
            tableMAXP->maxStackElements,
            tableMAXP->maxSizeOfInstructions,
            tableMAXP->maxComponentElements,
            tableMAXP->maxComponentDepth
        );
    }
}

OTFTableHMTX* FontParser_acquire_table_hmtx(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableHHEA* tableHHEA, OTFTableMAXP* tableMAXP) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'h', 'm', 't', 'x'});

    uint32 hmtx_offset = tableRec->offset;
    uint32 hmtx_length = tableRec->length;

    OTFTableHMTX* tableHMTX = (OTFTableHMTX*)malloc(sizeof(OTFTableHMTX));

    uint8_t* hmtx_ptr = ttf_file + hmtx_offset;

    tableHMTX->hMetrics = (LongHorMetric*)malloc(tableHHEA->numberOfHMetrics * sizeof(LongHorMetric));
    tableHMTX->leftSideBearings = (FWORD*)malloc((tableMAXP->numGlyphs - tableHHEA->numberOfHMetrics) * sizeof(FWORD));

    for (uint32 i = 0; i < tableHHEA->numberOfHMetrics; i++) {
        tableHMTX->hMetrics[i].advanceWidth = advance_16b(&hmtx_ptr);
        tableHMTX->hMetrics[i].lsb = advance_16b(&hmtx_ptr);
    }

    for (uint32 i = 0; i < tableMAXP->numGlyphs - tableHHEA->numberOfHMetrics; i++) {
        tableHMTX->leftSideBearings[i] = advance_16b(&hmtx_ptr);
    }

    return tableHMTX;
}

void FontParser_release_table_hmtx(OTFTableHMTX** tableHMTX) {
    free((*tableHMTX)->hMetrics);
    free((*tableHMTX)->leftSideBearings);
    free(*tableHMTX);
    *tableHMTX = NULL;
}

void FontParser_print_table_hmtx(OTFTableHMTX* tableHMTX, OTFTableHHEA* tableHHEA, OTFTableMAXP* tableMAXP, FILE* output) {
    fprintf(output, "Table HMTX {\n");

    fprintf(output, "    hMetrics {\n");

    for (uint32 i = 0; i < tableHHEA->numberOfHMetrics; i++) {
        fprintf(
            output,
            "       {\n"
            "           advanceWidth: %u\n"
            "           lsb: %d\n"
            "       }\n",
            tableHMTX->hMetrics[i].advanceWidth,
            tableHMTX->hMetrics[i].lsb
        );
    }

    fprintf(output, "    }\n");

    fprintf(output, "    leftSideBearings: {\n");

    for (uint32 i = 0; i < tableMAXP->numGlyphs - tableHHEA->numberOfHMetrics; i++) {
        if (i % 4 == 0) {
            fprintf(output, "\n      ");
        }

        fprintf(output, "%d, ", tableHMTX->leftSideBearings[i]);
    }

    fprintf(output, "    }\n");

    fprintf(output, "}\n");
}

OTFTableCMAP* FontParser_acquire_table_cmap(uint8_t* ttf_file, OTFTableDirectory* tableDir) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'c', 'm', 'a', 'p'});

    uint32 cmap_offset = tableRec->offset;
    uint32 cmap_length = tableRec->length;

    OTFTableCMAP* table_cmap = (OTFTableCMAP*)malloc(sizeof(OTFTableCMAP));

    uint8_t* cmap_ptr = ttf_file + cmap_offset;

    table_cmap->version = advance_16b(&cmap_ptr);
    table_cmap->numTables = advance_16b(&cmap_ptr);
    table_cmap->encodingRecords = (EncodingRecord*)malloc(table_cmap->numTables * sizeof(EncodingRecord));

    for (uint32 i = 0; i < table_cmap->numTables; i++) {
        table_cmap->encodingRecords[i].platformID = advance_16b(&cmap_ptr);
        table_cmap->encodingRecords[i].encodingID = advance_16b(&cmap_ptr);
        table_cmap->encodingRecords[i].subtableOffset = advance_32b(&cmap_ptr);
    }

    return table_cmap;
}

void FontParser_release_table_cmap(OTFTableCMAP** tableCMAP) {
    free((*tableCMAP)->encodingRecords);
    free(*tableCMAP);
    *tableCMAP = NULL;
}

void FontParser_print_table_cmap(OTFTableCMAP* tableCMAP, FILE* output) {
    fprintf(
        output,
        "Table CMAP {\n"
        "   version: %u\n"
        "   numTables: %u\n"
        "   encodingRecords {\n",
        tableCMAP->version,
        tableCMAP->numTables
    );

    for (uint32 i = 0; i < tableCMAP->numTables; i++) {
        fprintf(
            output,
            "       {\n"
            "           platformID: %u\n"
            "           encodingID: %u\n"
            "           subtableOffset: %u\n"
            "       }\n",
            tableCMAP->encodingRecords[i].platformID,
            tableCMAP->encodingRecords[i].encodingID,
            tableCMAP->encodingRecords[i].subtableOffset
        );
    }

    fprintf(output, "   }\n}\n");
}

OTFCMAPFormat4* FontParser_acquire_cmap_format4(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableCMAP* tableCMAP) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'c', 'm', 'a', 'p'});

    uint32 cmap_offset = tableRec->offset;
    uint32 cmap_length = tableRec->length;

    uint32 format4_offset = 0;
    for (uint32 i = 0; i < tableCMAP->numTables; i++) {
        if (
            (
                tableCMAP->encodingRecords[i].platformID == 3 &&
                tableCMAP->encodingRecords[i].encodingID == 1
            ) ||
            (
                tableCMAP->encodingRecords[i].platformID == 0 &&
                tableCMAP->encodingRecords[i].encodingID == 3
            )
        ) {
            format4_offset = tableCMAP->encodingRecords[i].subtableOffset;
            break;
        }
    }

    // This means none of the supported platformID and encodingID combos necessary for format4 were found
    if (format4_offset == 0) {
        return NULL;
    }

    uint8_t* format4_ptr = ttf_file + cmap_offset + format4_offset;

    OTFCMAPFormat4* format4_table = (OTFCMAPFormat4*)malloc(sizeof(OTFCMAPFormat4));

    format4_table->format = advance_16b(&format4_ptr);
    format4_table->length = advance_16b(&format4_ptr);
    format4_table->language = advance_16b(&format4_ptr);
    format4_table->segCountX2 = advance_16b(&format4_ptr);
    format4_table->searchRange = advance_16b(&format4_ptr);
    format4_table->entrySelector = advance_16b(&format4_ptr);
    format4_table->rangeShift = advance_16b(&format4_ptr);

    uint32 segCount = format4_table->segCountX2 / 2;

    format4_table->arena = (uint8_t*)malloc(format4_table->length - 8 * sizeof(uint16));

    uint16_t* arena_ptr = (uint16_t*)format4_table->arena;

    format4_table->endCode = (uint16*)arena_ptr;
    arena_ptr += segCount;

    for (uint32 i = 0; i < segCount; i++) {
        format4_table->endCode[i] = advance_16b(&format4_ptr);
    }

    format4_table->reservePad = advance_16b(&format4_ptr);

    format4_table->startCode = (uint16*)arena_ptr;
    arena_ptr += segCount;

    for (uint32 i = 0; i < segCount; i++) {
        format4_table->startCode[i] = advance_16b(&format4_ptr);
    }

    format4_table->idDelta = (int16*)arena_ptr;
    arena_ptr += segCount;

    for (uint32 i = 0; i < segCount; i++) {
        format4_table->idDelta[i] = (int16)advance_16b(&format4_ptr);
    }

    format4_table->idRangeOffset = (uint16*)arena_ptr;
    arena_ptr += segCount;

    for (uint32 i = 0; i < segCount; i++) {
        format4_table->idRangeOffset[i] = advance_16b(&format4_ptr);
    }

    // For valid font files this should work just fine, but corrupted files could
    // cause this to underflow
    uint32 glyphIDArrayLen = (format4_table->length - 4 * 2 * segCount - 8 * 2) / 2;

    format4_table->glyphIdArray = (uint16*)arena_ptr;

    for (uint32 i = 0; i < glyphIDArrayLen; i++) {
        format4_table->glyphIdArray[i] = advance_16b(&format4_ptr);
    }

    return format4_table;
}

void FontParser_release_cmap_format4(OTFCMAPFormat4** cmap_format4) {
    free((*cmap_format4)->arena);
    free(*cmap_format4);
    *cmap_format4 = NULL;
}

void FontParser_print_cmap_format4(OTFCMAPFormat4* cmap_format4, FILE* output) {
    fprintf(
        output,
        "CMAP Format4 {\n"
        "   format: %u\n"
        "   length: %u\n"
        "   language: %u\n"
        "   segCountX2: %u\n"
        "   searchRange: %u\n"
        "   entrySelector: %u\n"
        "   rangeShift: %u\n"
        "   endCode {\n",
        cmap_format4->format,
        cmap_format4->length,
        cmap_format4->language,
        cmap_format4->segCountX2,
        cmap_format4->searchRange,
        cmap_format4->entrySelector,
        cmap_format4->rangeShift
    );

    uint32 segCount = cmap_format4->segCountX2 / 2;
    uint32 numbers_per_line = 10;

    for (uint32 i = 0; i < segCount; i++) {
        if (i != 0 && i % numbers_per_line == 0) {
            fprintf(output, "\n");
        }

        fprintf(output, "       %6u, ", cmap_format4->endCode[i]);
    }

    fprintf(
        output,
        "\n"
        "   }\n"
        "   startCode {\n"
    );

    for (uint32 i = 0; i < segCount; i++) {
        if (i != 0 && i % numbers_per_line == 0) {
            fprintf(output, "\n");
        }

        fprintf(output, "       %6u, ", cmap_format4->startCode[i]);
    }

    fprintf(
        output,
        "\n"
        "   }\n"
        "   idDelta {\n"
    );

    for (uint32 i = 0; i < segCount; i++) {
        if (i != 0 && i % numbers_per_line == 0) {
            fprintf(output, "\n");
        }

        fprintf(output, "       %6d, ", cmap_format4->idDelta[i]);
    }

    fprintf(
        output,
        "\n"
        "   }\n"
        "   idRangeOffset {\n"
    );

    for (uint32 i = 0; i < segCount; i++) {
        if (i != 0 && i % numbers_per_line == 0) {
            fprintf(output, "\n");
        }

        fprintf(output, "       %6u, ", cmap_format4->idRangeOffset[i]);
    }

    fprintf(
        output,
        "\n"
        "   }\n"
        "   glyphIdArray {\n"
    );

    uint32 glyphIDArrayLen = (cmap_format4->length - 4 * 2 * segCount - 8 * 2) / 2;

    for (uint32 i = 0; i < glyphIDArrayLen; i++) {
        if (i != 0 && i % numbers_per_line == 0) {
            fprintf(output, "\n");
        }

        fprintf(output, "       %6u, ", cmap_format4->glyphIdArray[i]);
    }

    fprintf(
        output,
        "\n"
        "   }\n"
        "}\n"
    );
}

OTFTableLOCA* FontParser_acquire_table_loca(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableHEAD* tableHEAD, OTFTableMAXP* tableMAXP) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'l', 'o', 'c', 'a'});

    uint32 loca_offset = tableRec->offset;
    uint32 loca_length = tableRec->length;

    OTFTableLOCA* table_loca = (OTFTableLOCA*)malloc(sizeof(OTFTableLOCA));

    uint8_t* loca_ptr = ttf_file + loca_offset;

    // 0 means use short offsets and 1 means use long offsets
    if (tableHEAD->indexToLocFormat == 0) {
        table_loca->offsets16 = (Offset16*)malloc((tableMAXP->numGlyphs + 1) * sizeof(Offset16));

        for (uint32 i = 0; i < tableMAXP->numGlyphs + 1; i++) {
            table_loca->offsets16[i] = advance_16b(&loca_ptr);
        }
    } else if (tableHEAD->indexToLocFormat == 1) {
        table_loca->offsets32 = (Offset32*)malloc((tableMAXP->numGlyphs + 1) * sizeof(Offset32));

        for (uint32 i = 0; i < tableMAXP->numGlyphs + 1; i++) {
            table_loca->offsets32[i] = advance_32b(&loca_ptr);
        }
    } else {
        free(table_loca);
        return NULL;
    }

    return table_loca;
}

void FontParser_release_table_loca(OTFTableLOCA** tableLOCA) {
    // Because the struct only contains an anonymous union of two pointers
    // either the offsets16 or offsets32 can be freed since they would both point to the same 
    // memory
    free((*tableLOCA)->offsets16);
    free(*tableLOCA);
    *tableLOCA = NULL;
}

void FontParser_print_table_loca(OTFTableLOCA* tableLOCA, OTFTableHEAD* tableHEAD, OTFTableMAXP* tableMAXP, FILE* output) {
    fprintf(
        output,
        "Table LOCA: {\n"
        "   offsets {\n"
    );

    const uint32 numbers_per_line = 10;

    // 0 means short offsets and 1 means long offsets
    if (tableHEAD->indexToLocFormat == 0) {
        for (uint32 i = 0; i < tableMAXP->numGlyphs + 1; i++) {
            if (i != 0 && i % numbers_per_line == 0) {
                fprintf(output, "\n");
            }

            fprintf(output, "       %7u, ", tableLOCA->offsets16[i]);
        }
    } else if (tableHEAD->indexToLocFormat == 1) {
        for (uint32 i = 0; i < tableMAXP->numGlyphs + 1; i++) {
            if (i != 0 && i % numbers_per_line == 0) {
                fprintf(output, "\n");
            }

            fprintf(output, "       %7u, ", tableLOCA->offsets32[i]);
        }
    }

    if (tableMAXP->numGlyphs % numbers_per_line == 0) {
        fprintf(
            output,
            "   }\n}\n"
        );
    } else {
        fprintf(
            output,
            "\n"
            "   }\n}\n"
        );
    }
}

OTFTableGLYF* FontParser_acquire_table_glyf(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableMAXP* tableMAXP, OTFTableLOCA* tableLOCA, OTFTableHEAD* tableHEAD) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'g', 'l', 'y', 'f'});

    uint32 glyf_offset = tableRec->offset;
    uint32 glyf_length = tableRec->length;

    OTFTableGLYF* table_glyf = (OTFTableGLYF*)malloc(sizeof(OTFTableGLYF));

    table_glyf->numGlyphs = tableMAXP->numGlyphs;
    table_glyf->glyphs = (Glyph*)malloc(tableMAXP->numGlyphs * sizeof(Glyph));

    uint8_t* glyf_ptr = ttf_file + glyf_offset;

    for (uint32 i = 0; i < tableMAXP->numGlyphs; i++) {
        uint32 glyphSize = 0;
        if (tableHEAD->indexToLocFormat == 0) {
            // short offsets (these store the offset divided by two)
            glyphSize = 2 * (uint32)tableLOCA->offsets16[i + 1] - 2 * (uint32)tableLOCA->offsets16[i];

            // Due to glyfs being aligned to 2 byte boundaries, there could be extra bytes at
            // the end of the glyf that don't need to be parsed. This means it is easier to manually set
            // the glyf ptr according to the loca offsets each iteration
            glyf_ptr = (uint8*)(ttf_file + (uint64_t)glyf_offset + (uint64_t)tableLOCA->offsets16[i] * 2);
        } else if (tableHEAD->indexToLocFormat == 1) {
            // long offsets
            glyphSize = tableLOCA->offsets32[i + 1] - tableLOCA->offsets32[i];

            // Due to glyfs being aligned to 2 byte boundaries, there could be extra bytes at
            // the end of the glyf that don't need to be parsed. This means it is easier to manually set
            // the glyf ptr according to the loca offsets each iteration
            glyf_ptr = (uint8*)(ttf_file + (uint64_t)glyf_offset + (uint64_t)tableLOCA->offsets32[i]);
        }

        // This is allowed for some reason, and it basically just means to skip over these glyphs. 
        // I think these types of glyphs with a size of 0 are just meant to be white spaces
        if (glyphSize == 0) {
            table_glyf->glyphs[i].header.numberOfContours = 0;
            table_glyf->glyphs[i].header.xMin = 0;
            table_glyf->glyphs[i].header.yMin = 0;
            table_glyf->glyphs[i].header.xMax = 0;
            table_glyf->glyphs[i].header.yMax = 0;
            table_glyf->glyphs[i].sg.endPtsOfContours = NULL;
            table_glyf->glyphs[i].sg.flags = NULL;
            table_glyf->glyphs[i].sg.instructionLength = 0;
            table_glyf->glyphs[i].sg.instructions = NULL;
            table_glyf->glyphs[i].sg.numPoints = 0;
            table_glyf->glyphs[i].sg.xCoordinates = NULL;
            table_glyf->glyphs[i].sg.yCoordinates = NULL;
            continue;
        }

        table_glyf->glyphs[i].header.numberOfContours = advance_16b(&glyf_ptr);
        table_glyf->glyphs[i].header.xMin = advance_16b(&glyf_ptr);
        table_glyf->glyphs[i].header.yMin = advance_16b(&glyf_ptr);
        table_glyf->glyphs[i].header.xMax = advance_16b(&glyf_ptr);
        table_glyf->glyphs[i].header.yMax = advance_16b(&glyf_ptr);

        if (table_glyf->glyphs[i].header.numberOfContours > 0) {
            // Handle the case for a simple glyph

            table_glyf->glyphs[i].sg.endPtsOfContours = (uint16*)malloc(table_glyf->glyphs[i].header.numberOfContours * sizeof(uint16));
            for (uint32 j = 0; j < table_glyf->glyphs[i].header.numberOfContours; j++) {
                table_glyf->glyphs[i].sg.endPtsOfContours[j] = advance_16b(&glyf_ptr);
            }

            table_glyf->glyphs[i].sg.instructionLength = advance_16b(&glyf_ptr);
            if (table_glyf->glyphs[i].sg.instructionLength == 0) {
                table_glyf->glyphs[i].sg.instructions = NULL;
            } else {
                table_glyf->glyphs[i].sg.instructions = (uint8*)malloc(table_glyf->glyphs[i].sg.instructionLength * sizeof(uint8));
                for (uint32 j = 0; j < table_glyf->glyphs[i].sg.instructionLength; j++) {
                    table_glyf->glyphs[i].sg.instructions[j] = advance_8b(&glyf_ptr);
                }
            }

            // The last element of the endPtsOfContours array gives the highest valued index of any ending point in a contour. Since this is an
            // index, you add 1 to this value to get the number of points
            uint32 numPoints = table_glyf->glyphs[i].sg.endPtsOfContours[table_glyf->glyphs[i].header.numberOfContours - 1] + 1;
            table_glyf->glyphs[i].sg.numPoints = numPoints;

            table_glyf->glyphs[i].sg.flags = (uint8*)malloc(numPoints * sizeof(uint8));

            uint32 pointCounter = 0;
            while (pointCounter < numPoints) {
                table_glyf->glyphs[i].sg.flags[pointCounter] = advance_8b(&glyf_ptr);

                if (table_glyf->glyphs[i].sg.flags[pointCounter] & SG_REPEAT_FLAG) {
                    // The number of repeats is the byte that directly comes after the current flag with this bit set
                    uint8 numRepeats = advance_8b(&glyf_ptr);

                    // The +1 in the memory offset comes from the fact that the number of repeats is the amount that byte repeats in addition
                    // to the initial occurence of that byte
                    memset(table_glyf->glyphs[i].sg.flags + pointCounter + 1, table_glyf->glyphs[i].sg.flags[pointCounter], numRepeats);
                    pointCounter += numRepeats;
                }

                pointCounter++;
            }

            table_glyf->glyphs[i].sg.xCoordinates = (int16*)malloc(numPoints * sizeof(int16));

            pointCounter = 0;
            while (pointCounter < numPoints) {
                if (table_glyf->glyphs[i].sg.flags[pointCounter] & SG_X_SHORT_VECTOR) {
                    if (table_glyf->glyphs[i].sg.flags[pointCounter] & SG_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR) {
                        // x coordinate is one byte long and positive
                        table_glyf->glyphs[i].sg.xCoordinates[pointCounter] = (int16)advance_8b(&glyf_ptr);
                    } else {
                        // x coordinate is one byte long and negative
                        table_glyf->glyphs[i].sg.xCoordinates[pointCounter] = -(int16)advance_8b(&glyf_ptr);
                    }
                } else {
                    if (table_glyf->glyphs[i].sg.flags[pointCounter] & SG_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR) {
                        // The coordinate remains unchanged, so the delta from the previous coordinate is just zero
                        table_glyf->glyphs[i].sg.xCoordinates[pointCounter] = 0;
                    } else {
                        // signed 16 bit (2 byte) delta vector
                        table_glyf->glyphs[i].sg.xCoordinates[pointCounter] = (int16)advance_16b(&glyf_ptr);
                    }
                }

                pointCounter++;
            }

            table_glyf->glyphs[i].sg.yCoordinates = (int16*)malloc(numPoints * sizeof(int16));

            pointCounter = 0;
            while (pointCounter < numPoints) {
                if (table_glyf->glyphs[i].sg.flags[pointCounter] & SG_Y_SHORT_VECTOR) {
                    if (table_glyf->glyphs[i].sg.flags[pointCounter] & SG_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR) {
                        // y coordinate is one byte long and positive
                        table_glyf->glyphs[i].sg.yCoordinates[pointCounter] = (int16)advance_8b(&glyf_ptr);
                    } else {
                        // y coordinate is one byte long and negative
                        table_glyf->glyphs[i].sg.yCoordinates[pointCounter] = -(int16)advance_8b(&glyf_ptr);
                    }
                } else {
                    if (table_glyf->glyphs[i].sg.flags[pointCounter] & SG_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR) {
                        // The coordinate remains unchanged, so delta from the previous coordinate is just 0
                        table_glyf->glyphs[i].sg.yCoordinates[pointCounter] = 0;
                    } else {
                        // signed 16 bit (2 byte) delta vector
                        table_glyf->glyphs[i].sg.yCoordinates[pointCounter] = (int16)advance_16b(&glyf_ptr);
                    }
                }

                pointCounter++;
            }
        } else if (table_glyf->glyphs[i].header.numberOfContours < 0) {
            // Handle the case for a composite glyph
            table_glyf->glyphs[i].cg.componentGlyphsLength = tableMAXP->maxComponentElements;

            // It could be worthwhile looking into ways to get more accurate sizing for the number of component glyphps, but for now the bit of extra memory
            // that comes with using the maximum number of component glyphs as a baseline should suffice
            table_glyf->glyphs[i].cg.componentGlyphs = (ComponentGlyphRecord*)malloc(table_glyf->glyphs[i].cg.componentGlyphsLength * sizeof(ComponentGlyphRecord));

            uint32 componentRecordCount = 0;

            uint16 flags;
            do {
                table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].flags = advance_16b(&glyf_ptr);
                flags = table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].flags;

                table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].glyphIndex = advance_16b(&glyf_ptr);

                if (flags & CG_ARG_1_AND_2_ARE_WORDS) {
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].argument1 = advance_16b(&glyf_ptr);
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].argument2 = advance_16b(&glyf_ptr);
                } else {
                    if (flags & CG_ARGS_ARE_XY_VALUES) {
                        // If the arguments are offsets, then the byte values are signed 8 bit integers
                        table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].argument1 = (int8_t)advance_8b(&glyf_ptr);
                        table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].argument2 = (int8_t)advance_8b(&glyf_ptr);
                    } else {
                        table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].argument1 = advance_8b(&glyf_ptr);
                        table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].argument2 = advance_8b(&glyf_ptr);
                    }
                }

                if (flags & CG_WE_HAVE_A_SCALE) {
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].scale = (F2DOT14)advance_16b(&glyf_ptr);
                } else if (flags & CG_WE_HAVE_AN_X_AND_Y_SCALE) {
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].xscale = (F2DOT14)advance_16b(&glyf_ptr);
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].yscale = (F2DOT14)advance_16b(&glyf_ptr);
                } else if (flags & CG_WE_HAVE_A_TWO_BY_TWO) {
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].xscale = (F2DOT14)advance_16b(&glyf_ptr);
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].scale01 = (F2DOT14)advance_16b(&glyf_ptr);
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].scale10 = (F2DOT14)advance_16b(&glyf_ptr);
                    table_glyf->glyphs[i].cg.componentGlyphs[componentRecordCount].yscale = (F2DOT14)advance_16b(&glyf_ptr);
                }
                
                componentRecordCount++;
            } while (flags & CG_MORE_COMPONENTS);

            table_glyf->glyphs[i].cg.componentGlyphsLength = componentRecordCount;

            table_glyf->glyphs[i].cg.instructions = NULL;
            table_glyf->glyphs[i].cg.numInstructions = 0;
            if (flags & CG_WE_HAVE_INSTRUCTIONS) {
                table_glyf->glyphs[i].cg.numInstructions = advance_16b(&glyf_ptr);
                table_glyf->glyphs[i].cg.instructions = (uint8*)malloc(table_glyf->glyphs[i].cg.numInstructions * sizeof(uint8));
                for (uint32 j = 0; j < table_glyf->glyphs[i].cg.numInstructions; j++) {
                    table_glyf->glyphs[i].cg.instructions[j] = advance_8b(&glyf_ptr);
                }
            }
        } else {
            // In the case where the number of contours is 0, that just means there is no point data. It's pretty much just a blank space
            // It's still classified as a simple glyph, but it is easier to handle it as a special case
            table_glyf->glyphs[i].sg.endPtsOfContours = NULL;
            table_glyf->glyphs[i].sg.flags = NULL;
            table_glyf->glyphs[i].sg.xCoordinates = NULL;
            table_glyf->glyphs[i].sg.yCoordinates = NULL;
            table_glyf->glyphs[i].sg.numPoints = 0;
            table_glyf->glyphs[i].sg.instructionLength = 0;
            table_glyf->glyphs[i].sg.instructions = NULL;

            // Interestingly enough, even with no contours a simple glyph can still technically have instructions
            // However, the only way to verify if there are instructions is to use the loca table to calculate the size of the glyf

            // The header for the glyph itself is just 10 bytes, so if the glyph size is greater than that, it means there must be instructions
            if (glyphSize > 10) {
                table_glyf->glyphs[i].sg.instructionLength = advance_16b(&glyf_ptr);
                table_glyf->glyphs[i].sg.instructions = (uint8*)malloc(table_glyf->glyphs[i].sg.instructionLength * sizeof(uint8));
                for (uint32 j = 0; j < table_glyf->glyphs[i].sg.instructionLength; j++) {
                    table_glyf->glyphs[i].sg.instructions[j] = advance_8b(&glyf_ptr);
                }
            }
        }
    }

    return table_glyf;
}

void FontParser_release_table_glyf(OTFTableGLYF** tableGLYF) {
    OTFTableGLYF* table_glyf = *tableGLYF;

    for (uint32 i = 0; i < table_glyf->numGlyphs; i++) {
        if (table_glyf->glyphs[i].header.numberOfContours >= 0) {
            // This means the simple glyph will have to be freed
            free(table_glyf->glyphs[i].sg.endPtsOfContours);
            free(table_glyf->glyphs[i].sg.flags);
            free(table_glyf->glyphs[i].sg.instructions);
            free(table_glyf->glyphs[i].sg.xCoordinates);
            free(table_glyf->glyphs[i].sg.yCoordinates);
        } else {
            // This means the composite glyph will have to be freed
            free(table_glyf->glyphs[i].cg.componentGlyphs);
            free(table_glyf->glyphs[i].cg.instructions);
        }
    }

    free(table_glyf->glyphs);
    free(*tableGLYF);
    *tableGLYF = NULL;
}

void FontParser_print_table_glyf(OTFTableGLYF* tableGLYF, FILE* output) {
    const uint32 numbers_per_line = 10;
    fprintf(
        output,
        "Table GLYF: {\n"
    );

    for (uint32 i = 0; i < tableGLYF->numGlyphs; i++) {
        if (tableGLYF->glyphs[i].header.numberOfContours >= 0) {
            // Simple glyph case
            fprintf(
                output,
                "   Simple Glyph {\n"
                "       endPtsOfContours {\n"
            );

            for (uint32 j = 0; j < tableGLYF->glyphs[i].header.numberOfContours; j++) {
                if (j != 0 && j % numbers_per_line == 0) {
                    fprintf(output, "\n");
                }

                fprintf(output, "           %4u, ", tableGLYF->glyphs[i].sg.endPtsOfContours[j]);
            }

            fprintf(output, "\n       }\n");

            fprintf(
                output,
                "       instructionLength: %u\n"
                "       instructions {\n",
                tableGLYF->glyphs[i].sg.instructionLength
            );

            for (uint32 j = 0; j < tableGLYF->glyphs[i].sg.instructionLength; j++) {
                if (j != 0 && j % numbers_per_line == 0) {
                    fprintf(output, "\n");
                }

                fprintf(output, "           %4u, ", tableGLYF->glyphs[i].sg.instructions[j]);
            }

            fprintf(output, "\n       }\n");


            fprintf(
                output,
                "       flags {\n"
            );

            for (uint32 j = 0; j < tableGLYF->glyphs[i].sg.numPoints; j++) {
                if (j != 0 && j % numbers_per_line == 0) {
                    fprintf(output, "\n");
                }

                fprintf(output, "           %4s0x%X, ", "", tableGLYF->glyphs[i].sg.flags[j]);
            }

            fprintf(
                output,
                "\n       }\n"
                "       xCoordinates {\n"
            );

            for (uint32 j = 0; j < tableGLYF->glyphs[i].sg.numPoints; j++) {
                if (j != 0 && j % numbers_per_line == 0) {
                    fprintf(output, "\n");
                }

                fprintf(output, "           %6d, ", tableGLYF->glyphs[i].sg.xCoordinates[j]);
            }

            fprintf(
                output,
                "\n       }\n"
                "       yCoordinates {\n"
            );

            for (uint32 j = 0; j < tableGLYF->glyphs[i].sg.numPoints; j++) {
                if (j != 0 && j % numbers_per_line == 0) {
                    fprintf(output, "\n");
                }

                fprintf(output, "           %6d, ", tableGLYF->glyphs[i].sg.yCoordinates[j]);
            }

            fprintf(
                output,
                "\n       }\n"
                "       numPoints: %u\n"
                "   }\n",
                tableGLYF->glyphs[i].sg.numPoints
            );
        } else {
            // Composite glyph case
            fprintf(
                output,
                "   Composite Glyph {\n"
                "       componentGlyphs {\n"
            );

            for (uint32 j = 0; j < tableGLYF->glyphs[i].cg.componentGlyphsLength; j++) {
                fprintf(
                    output,
                    "           {\n"
                    "               flags: %X\n"
                    "               glyphIndex: %u\n"
                    "               argument1: %u\n"
                    "               argument2: %u\n",
                    tableGLYF->glyphs[i].cg.componentGlyphs[j].flags,
                    tableGLYF->glyphs[i].cg.componentGlyphs[j].glyphIndex,
                    tableGLYF->glyphs[i].cg.componentGlyphs[j].argument1,
                    tableGLYF->glyphs[i].cg.componentGlyphs[j].argument2
                );

                if (tableGLYF->glyphs[i].cg.componentGlyphs[j].flags & CG_WE_HAVE_A_SCALE) {
                    fprintf(output, "              scale: %d\n", tableGLYF->glyphs[i].cg.componentGlyphs[j].scale);
                } else if (tableGLYF->glyphs[i].cg.componentGlyphs[j].flags & CG_WE_HAVE_AN_X_AND_Y_SCALE) {
                    fprintf(output, "              xscale: %d\n", tableGLYF->glyphs[i].cg.componentGlyphs[j].xscale);
                    fprintf(output, "              yscale: %d\n", tableGLYF->glyphs[i].cg.componentGlyphs[j].yscale);
                } else if (tableGLYF->glyphs[i].cg.componentGlyphs[j].flags & CG_WE_HAVE_A_TWO_BY_TWO) {
                    fprintf(output, "              xscale: %d\n", tableGLYF->glyphs[i].cg.componentGlyphs[j].xscale);
                    fprintf(output, "              scale01: %d\n", tableGLYF->glyphs[i].cg.componentGlyphs[j].scale01);
                    fprintf(output, "              scale10: %d\n", tableGLYF->glyphs[i].cg.componentGlyphs[j].scale10);
                    fprintf(output, "              yscale: %d\n", tableGLYF->glyphs[i].cg.componentGlyphs[j].yscale);
                }

                fprintf(output, "           }\n");
            }

            fprintf(
                output, 
                "       }\n"
                "       componentGlyphsLength: %u\n"
                "       numInstructions: %u\n"
                "       instructions {\n",
                tableGLYF->glyphs[i].cg.componentGlyphsLength,
                tableGLYF->glyphs[i].cg.numInstructions
            );

            for (uint32 j = 0; j < tableGLYF->glyphs[i].cg.numInstructions; j++) {
                if (j != 0 && j % numbers_per_line == 0) {
                    fprintf(output, "\n");
                }

                fprintf(output, "           %4u, ", tableGLYF->glyphs[i].cg.instructions[j]);
            }

            fprintf(
                output,
                "\n       }\n"
                "   }\n"
            );
        }
    }

    fprintf(
        output,
        "}\n"
    );
}

OTFFontFile* FontParser_acquire_font(const char* font_file) {
    FILE* filePointer = fopen(font_file, "rb");

    if (filePointer == NULL) {
        printf("Failed to load font.\n");
        return NULL;
    }

    fseek(filePointer, 0L, SEEK_END);
    uint64_t fileSize = ftell(filePointer);
    rewind(filePointer);

    uint8_t* ttf_file = (uint8_t*)malloc(fileSize);

    fread(ttf_file, 1, fileSize, filePointer);

    fclose(filePointer);

    OTFFontFile* otf_font = (OTFFontFile*)malloc(sizeof(OTFFontFile));

    OTFTableDirectory* tbdir = FontParser_acquire_table_directory(ttf_file);

    otf_font->head = FontParser_acquire_table_head(ttf_file, tbdir);
    otf_font->hhea = FontParser_acquire_table_hhea(ttf_file, tbdir);
    otf_font->maxp = FontParser_acquire_table_maxp(ttf_file, tbdir);
    otf_font->hmtx = FontParser_acquire_table_hmtx(ttf_file, tbdir, otf_font->hhea, otf_font->maxp);
    otf_font->cmap = FontParser_acquire_table_cmap(ttf_file, tbdir);
    otf_font->format4 = FontParser_acquire_cmap_format4(ttf_file, tbdir, otf_font->cmap);
    otf_font->loca = FontParser_acquire_table_loca(ttf_file, tbdir, otf_font->head, otf_font->maxp);
    otf_font->glyf = FontParser_acquire_table_glyf(ttf_file, tbdir, otf_font->maxp, otf_font->loca, otf_font->head);

    FontParser_release_table_directory(&tbdir);
    free(ttf_file);

    return otf_font;
}

void FontParser_release_font(OTFFontFile** font_file) {
    FontParser_release_table_head(&(*font_file)->head);
    FontParser_release_table_hhea(&(*font_file)->hhea);
    FontParser_release_table_maxp(&(*font_file)->maxp);
    FontParser_release_table_hmtx(&(*font_file)->hmtx);
    FontParser_release_table_cmap(&(*font_file)->cmap);
    FontParser_release_cmap_format4(&(*font_file)->format4);
    FontParser_release_table_loca(&(*font_file)->loca);
    FontParser_release_table_glyf(&(*font_file)->glyf);
    free(*font_file);
    *font_file = NULL;
}

void FontParser_print_font(OTFFontFile* font_file, FILE* output) {
    FontParser_print_table_head(font_file->head, output);
    FontParser_print_table_hhea(font_file->hhea, output);
    FontParser_print_table_maxp(font_file->maxp, output);
    FontParser_print_table_hmtx(font_file->hmtx, font_file->hhea, font_file->maxp, output);
    FontParser_print_table_cmap(font_file->cmap, output);
    FontParser_print_cmap_format4(font_file->format4, output);
    FontParser_print_table_loca(font_file->loca, font_file->head, font_file->maxp, output);
    FontParser_print_table_glyf(font_file->glyf, output);
}

uint32_t FontParser_get_glyphID(OTFFontFile* font_file, uint32 character) {
    uint32 segments = font_file->format4->segCountX2 / 2;

    // Get the index of the first end code greater than or equal to character
    int64_t endCodeIndex = -1;
    for (uint32 i = 0; i < segments; i++) {
        if (font_file->format4->endCode[i] >= character) {
            endCodeIndex = i;
            break;
        }
    }

    if (endCodeIndex < 0) {
        return 0;
    }

    // Make sure the character falls within the range of the segment
    if (font_file->format4->startCode[endCodeIndex] <= character) {
        if (font_file->format4->idRangeOffset[endCodeIndex] == 0) {
            int32_t glyphID = (int32_t)font_file->format4->idDelta[endCodeIndex] + (int32_t)character;

            if (glyphID > 0) {
                return (uint32_t)glyphID;
            } else {
                return (uint32)glyphID + 65536;
            }
        } else {
            return *(font_file->format4->idRangeOffset[endCodeIndex]/2 + (character - font_file->format4->startCode[endCodeIndex]) + &font_file->format4->idRangeOffset[endCodeIndex]);
        }
    } else {
        // This is the id for the missing glyph
        return 0;
    }
}