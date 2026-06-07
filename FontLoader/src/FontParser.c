#include "FontParser.h"
// This must come after FontParser.h
#include "FontTypesPrivate.h"
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
    (*tableLOCA)->offsets16 = NULL;
}

void FontParser_print_table_loca(OTFTableLOCA* tableLOCA, OTFTableHEAD* tableHEAD, OTFTableMAXP* tableMAXP, FILE* output) {
    fprintf(
        output,
        "Table LOCA: {\n"
        "   offsets {\n"
    );

    // 0 means short offsets and 1 means long offsets
    if (tableHEAD->indexToLocFormat == 0) {
        for (uint32 i = 0; i < tableMAXP->numGlyphs + 1; i++) {
            fprintf(output, "       %u\n", tableLOCA->offsets16[i]);
        }
    } else if (tableHEAD->indexToLocFormat == 1) {
        for (uint32 i = 0; i < tableMAXP->numGlyphs + 1; i++) {
            fprintf(output, "       %u\n", tableLOCA->offsets32[i]);
        }
    }

    fprintf(
        output,
        "   }\n}\n"
    );
}

OTFTableGLYF* FontParser_acquire_table_glyf(uint8_t* ttf_file, OTFTableDirectory* tableDir, OTFTableMAXP* tableMAXP) {
    TableRecord* tableRec = get_table_record(tableDir, (Tag){'g', 'l', 'y', 'f'});

    uint32 glyf_offset = tableRec->offset;
    uint32 glyf_length = tableRec->length;

    OTFTableGLYF* table_glyf = (OTFTableGLYF*)malloc(sizeof(OTFTableGLYF));

    table_glyf->glyphs = (Glyph*)malloc(tableMAXP->numGlyphs * sizeof(Glyph));

    uint8_t* glyf_ptr = ttf_file + glyf_offset;

    for (uint32 i = 0; i < tableMAXP->numGlyphs; i++) {
        table_glyf->glyphs[i].header.numberOfContours = advance_16b(&glyf_ptr);
        table_glyf->glyphs[i].header.xMin = advance_16b(&glyf_ptr);
        table_glyf->glyphs[i].header.yMin = advance_16b(&glyf_ptr);
        table_glyf->glyphs[i].header.xMax = advance_16b(&glyf_ptr);
        table_glyf->glyphs[i].header.yMax = advance_16b(&glyf_ptr);

        if (table_glyf->glyphs[i].header.numberOfContours >= 0) {
            // Handle the case for a simple glyph
            table_glyf->glyphs[i].sg.endPtsOfContours = (uint16*)malloc(table_glyf->glyphs[i].header.numberOfContours * sizeof(uint16));
            memcpy(table_glyf->glyphs[i].sg.endPtsOfContours, glyf_ptr, table_glyf->glyphs[i].header.numberOfContours * sizeof(uint16));
            glyf_ptr += table_glyf->glyphs[i].header.numberOfContours * sizeof(uint16);

            table_glyf->glyphs[i].sg.instructionLength = advance_16b(&glyf_ptr);
            if (table_glyf->glyphs[i].sg.instructionLength == 0) {
                table_glyf->glyphs[i].sg.instructions = NULL;
            } else {
                table_glyf->glyphs[i].sg.instructions = (uint8*)malloc(table_glyf->glyphs[i].sg.instructionLength * sizeof(uint8));
                memcpy(table_glyf->glyphs[i].sg.instructions, glyf_ptr, table_glyf->glyphs[i].sg.instructionLength * sizeof(uint8));
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
        } else {
            // Handle the case for a composite glyph
        }
    }
}

void FontParser_release_table_glyf(OTFTableGLYF** tableGLYF) {

}

void FontParser_print_table_glyf(OTFTableGLYF* tableGLYF, OTFTableMAXP* tableMAXP, FILE* output) {

}