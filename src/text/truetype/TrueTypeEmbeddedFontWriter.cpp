/*
   Source File : TrueTypeEmbeddedFontWriter.cpp


   Copyright 2011 Gal Kahana PDFWriter

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


*/
#include "text/truetype/TrueTypeEmbeddedFontWriter.h"
#include "DictionaryContext.h"
#include "FSType.h"
#include "ObjectsContext.h"
#include "PDFStream.h"
#include "Trace.h"
#include "io/InputStringBufferStream.h"
#include "io/OutputStreamTraits.h"
#include "text/freetype/FreeTypeFaceWrapper.h"
#include "text/opentype/OpenTypeFileInput.h"

#include <algorithm>
#include <sstream>

using namespace charta;

TrueTypeEmbeddedFontWriter::TrueTypeEmbeddedFontWriter() : mFontFileReaderStream(nullptr)
{
}

TrueTypeEmbeddedFontWriter::~TrueTypeEmbeddedFontWriter() = default;

static const std::string scLength1 = "Length1";
EStatusCode TrueTypeEmbeddedFontWriter::WriteEmbeddedFont(FreeTypeFaceWrapper &inFontInfo,
                                                          const UIntVector &inSubsetGlyphIDs,
                                                          ObjectsContext *inObjectsContext,
                                                          ObjectIDType &outEmbeddedFontObjectID)
{
    MyStringBuf rawFontProgram;
    bool notEmbedded;
    EStatusCode status;

    do
    {
        status = CreateTrueTypeSubset(inFontInfo, inSubsetGlyphIDs, notEmbedded, rawFontProgram);
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::WriteEmbeddedFont, failed to write embedded font program");
            break;
        }

        if (notEmbedded)
        {
            // can't embed. mark succesful, and go back empty
            outEmbeddedFontObjectID = 0;
            TRACE_LOG("TrueTypeEmbeddedFontWriter::WriteEmbeddedFont, font may not be embedded. so not embedding");
            return charta::eSuccess;
        }

        outEmbeddedFontObjectID = inObjectsContext->StartNewIndirectObject();

        DictionaryContext *fontProgramDictionaryContext = inObjectsContext->StartDictionary();

        // Length1 (decompressed true type program length)

        fontProgramDictionaryContext->WriteKey(scLength1);
        fontProgramDictionaryContext->WriteIntegerValue(rawFontProgram.GetCurrentWritePosition());
        rawFontProgram.pubseekoff(0, std::ios_base::beg);
        std::shared_ptr<PDFStream> pdfStream = inObjectsContext->StartPDFStream(fontProgramDictionaryContext);

        // now copy the created font program to the output stream
        InputStringBufferStream fontProgramStream(&rawFontProgram);
        OutputStreamTraits streamCopier(pdfStream->GetWriteStream());
        status = streamCopier.CopyToOutputStream(&fontProgramStream);
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::WriteEmbeddedFont, failed to copy font program into pdf stream");
            break;
        }

        inObjectsContext->EndPDFStream(pdfStream);
    } while (false);

    return status;
}

EStatusCode TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset(
    FreeTypeFaceWrapper &inFontInfo, /*consider requiring only the file path...actually i don't need the whole thing*/
    const UIntVector &inSubsetGlyphIDs, bool &outNotEmbedded, MyStringBuf &outFontProgram)
{
    EStatusCode status;
    unsigned long *locaTable = nullptr;

    do
    {
        UIntVector subsetGlyphIDs = inSubsetGlyphIDs;

        status = mTrueTypeFile.OpenFile(inFontInfo.GetFontFilePath());
        if (status != charta::eSuccess)
        {
            TRACE_LOG1("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, cannot open true type font file at %s",
                       inFontInfo.GetFontFilePath().c_str());
            break;
        }

        status = mTrueTypeInput.ReadOpenTypeFile(mTrueTypeFile.GetInputStream(), (uint16_t)inFontInfo.GetFontIndex());
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to read true type file");
            break;
        }

        if (mTrueTypeInput.GetOpenTypeFontType() != EOpenTypeTrueType)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, font file is not true type, so there is an "
                      "exceptions here. expecting true types only");
            break;
        }

        // see if font may be embedded
        if (mTrueTypeInput.mOS2Exists && !FSType(mTrueTypeInput.mOS2.fsType).CanEmbed())
        {
            outNotEmbedded = true;
            return charta::eSuccess;
        }
        outNotEmbedded = false;

        AddDependentGlyphs(subsetGlyphIDs);

        // K. this needs a bit explaining.
        // i want to leave the glyph IDs as they were in the original font.
        // this allows me to write a more comfotable font definition. something which is generic enough
        // this assumption requires that the font will contain the glyphs in their original position
        // to allow that, when the glyph count is smaller than the actual glyphs count, i'm
        // padding with 0 length glyphs (their loca entries just don't move).
        // don't worry - it's perfectly kosher.
        // so - bottom line - the glyphs count will actually be 1 more than the maxium glyph index.
        // and from here i'll just place the glyphs in their original indexes, and fill in the
        // vacant glyphs with empties.
        mSubsetFontGlyphsCount = subsetGlyphIDs.back() + 1;

        mFontFileStream.Assign(&outFontProgram);
        mPrimitivesWriter.SetOpenTypeStream(&mFontFileStream);

        // assign also to some reader streams, so i can read items for checksums calculations
        mFontFileReaderStream.Assign(&outFontProgram);
        mPrimitivesReader.SetOpenTypeStream(&mFontFileReaderStream);

        status = WriteTrueTypeHeader();
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write true type header");
            break;
        }

        status = WriteHead();
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write head table");
            break;
        }

        status = WriteHHea();
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write hhea table");
            break;
        }

        status = WriteHMtx();
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write hmtx table");
            break;
        }

        status = WriteMaxp();
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write Maxp table");
            break;
        }

        if (mTrueTypeInput.mCVTExists)
        {
            status = WriteCVT();
            if (status != charta::eSuccess)
            {
                TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write cvt table");
                break;
            }
        }

        if (mTrueTypeInput.mFPGMExists)
        {
            status = WriteFPGM();
            if (status != charta::eSuccess)
            {
                TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write fpgm table");
                break;
            }
        }

        if (mTrueTypeInput.mPREPExists)
        {
            status = WritePREP();
            if (status != charta::eSuccess)
            {
                TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write prep table");
                break;
            }
        }

        status = WriteNAME();
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write name table");
            break;
        }

        if (mTrueTypeInput.mOS2Exists)
        {
            status = WriteOS2();
            if (status != charta::eSuccess)
            {
                TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write os2 table");
                break;
            }
        }

        status = WriteCMAP();
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write cmap table");
            break;
        }

        locaTable = new unsigned long[mSubsetFontGlyphsCount + 1];

        status = WriteGlyf(subsetGlyphIDs, locaTable);
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write prep table");
            break;
        }

        status = WriteLoca(locaTable);
        if (status != charta::eSuccess)
        {
            TRACE_LOG("TrueTypeEmbeddedFontWriter::CreateTrueTypeSubset, failed to write loca table");
            break;
        }

        status = CreateHeadTableCheckSumAdjustment();
    } while (false);

    delete[] locaTable;
    mTrueTypeFile.CloseFile();
    return status;
}

void TrueTypeEmbeddedFontWriter::AddDependentGlyphs(UIntVector &ioSubsetGlyphIDs)
{
    UIntSet glyphsSet;
    auto it = ioSubsetGlyphIDs.begin();
    bool hasCompositeGlyphs = false;

    for (; it != ioSubsetGlyphIDs.end(); ++it)
        hasCompositeGlyphs |= AddComponentGlyphs(*it, glyphsSet);

    if (hasCompositeGlyphs)
    {
        UIntSet::iterator itNewGlyphs;

        for (it = ioSubsetGlyphIDs.begin(); it != ioSubsetGlyphIDs.end(); ++it)
            glyphsSet.insert(*it);

        ioSubsetGlyphIDs.clear();
        for (itNewGlyphs = glyphsSet.begin(); itNewGlyphs != glyphsSet.end(); ++itNewGlyphs)
            ioSubsetGlyphIDs.push_back(*itNewGlyphs);

        sort(ioSubsetGlyphIDs.begin(), ioSubsetGlyphIDs.end());
    }
}

bool TrueTypeEmbeddedFontWriter::AddComponentGlyphs(uint32_t inGlyphID, UIntSet &ioComponents)
{
    GlyphEntry *glyfTableEntry;
    UIntList::iterator itComponentGlyphs;
    bool isComposite = false;

    if (inGlyphID >= mTrueTypeInput.mMaxp.NumGlyphs)
    {
        TRACE_LOG2("TrueTypeEmbeddedFontWriter::AddComponentGlyphs, error, requested glyph index %ld is larger than "
                   "the maximum glyph index for this font which is %ld. ",
                   inGlyphID, mTrueTypeInput.mMaxp.NumGlyphs - 1);
        return false;
    }

    glyfTableEntry = mTrueTypeInput.mGlyf[inGlyphID];
    if (glyfTableEntry != nullptr && !glyfTableEntry->mComponentGlyphs.empty())
    {
        isComposite = true;
        for (itComponentGlyphs = glyfTableEntry->mComponentGlyphs.begin();
             itComponentGlyphs != glyfTableEntry->mComponentGlyphs.end(); ++itComponentGlyphs)
        {
            ioComponents.insert(*itComponentGlyphs);
            AddComponentGlyphs(*itComponentGlyphs, ioComponents);
        }
    }
    return isComposite;
}

uint16_t TrueTypeEmbeddedFontWriter::GetSmallerPower2(uint16_t inNumber)
{
    uint16_t comparer = inNumber > 0xff ? 0x8000 : 0x80;
    // that's 1 binary at the leftmost of the short or byte (most times there are less than 255 tables)
    uint32_t i = inNumber > 0xff ? 15 : 7;

    // now basically i'm gonna move the comparer down 1 bit every time, till i hit non 0, which
    // is the highest power

    while (comparer > 0 && ((inNumber & comparer) == 0))
    {
        comparer = comparer >> 1;
        --i;
    }
    return i;
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteTrueTypeHeader()
{
    // prepare space for tables to write. will write (at maximum) -
    // cmap, cvt, fpgm, glyf, head, hhea, hmtx, loca, maxp, name, os/2, prep

    uint16_t tableCount = 9 // needs - cmap, glyf, head, hhea, hmtx, loca, maxp, name, OS/2
                          + (mTrueTypeInput.mCVTExists ? 1 : 0) + // cvt
                          (mTrueTypeInput.mPREPExists ? 1 : 0) +  // prep
                          (mTrueTypeInput.mFPGMExists ? 1 : 0);   // fpgm

    // here we go....
    mPrimitivesWriter.WriteULONG(0x10000);
    mPrimitivesWriter.WriteUSHORT(tableCount);
    uint16_t smallerPowerTwo = GetSmallerPower2(tableCount);
    mPrimitivesWriter.WriteUSHORT(1 << (smallerPowerTwo + 4));
    mPrimitivesWriter.WriteUSHORT(smallerPowerTwo);
    mPrimitivesWriter.WriteUSHORT((tableCount - (1 << smallerPowerTwo)) << 4);

    if (mTrueTypeInput.mOS2Exists)
        WriteEmptyTableEntry("OS/2", mOS2EntryWritingOffset);
    WriteEmptyTableEntry("cmap", mCMAPEntryWritingOffset);
    if (mTrueTypeInput.mCVTExists)
        WriteEmptyTableEntry("cvt ", mCVTEntryWritingOffset);
    if (mTrueTypeInput.mFPGMExists)
        WriteEmptyTableEntry("fpgm", mFPGMEntryWritingOffset);
    WriteEmptyTableEntry("glyf", mGLYFEntryWritingOffset);
    WriteEmptyTableEntry("head", mHEADEntryWritingOffset);
    WriteEmptyTableEntry("hhea", mHHEAEntryWritingOffset);
    WriteEmptyTableEntry("hmtx", mHMTXEntryWritingOffset);
    WriteEmptyTableEntry("loca", mLOCAEntryWritingOffset);
    WriteEmptyTableEntry("maxp", mMAXPEntryWritingOffset);
    WriteEmptyTableEntry("name", mNAMEEntryWritingOffset);
    if (mTrueTypeInput.mPREPExists)
        WriteEmptyTableEntry("prep", mPREPEntryWritingOffset);

    mPrimitivesWriter.PadTo4();

    return mPrimitivesWriter.GetInternalState();
}

void TrueTypeEmbeddedFontWriter::WriteEmptyTableEntry(const char *inTag, long long &outEntryPosition)
{
    mPrimitivesWriter.WriteULONG(GetTag(inTag));
    outEntryPosition = mFontFileStream.GetCurrentPosition();
    mPrimitivesWriter.Pad(12);
}

unsigned long TrueTypeEmbeddedFontWriter::GetTag(const char *inTagName)
{
    uint8_t buffer[4];
    uint16_t i = 0;

    for (; i < strlen(inTagName); ++i)
        buffer[i] = (uint8_t)inTagName[i];
    for (; i < 4; ++i)
        buffer[i] = 0x20;

    return ((unsigned long)buffer[0] << 24) + ((unsigned long)buffer[1] << 16) + ((unsigned long)buffer[2] << 8) +
           buffer[3];
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteHead()
{
    // copy as is, then adjust loca table format to long (that's what i'm always writing),
    // set the checksum
    // and store the offset to the checksum

    TableEntry *tableEntry = mTrueTypeInput.GetTableEntry("head");
    long long startTableOffset;
    OutputStreamTraits streamCopier(&mFontFileStream);
    long long endOfStream;

    startTableOffset = mFontFileStream.GetCurrentPosition();

    // copy and save the current position
    mTrueTypeFile.GetInputStream()->SetPosition(tableEntry->Offset);
    streamCopier.CopyToOutputStream(mTrueTypeFile.GetInputStream(), tableEntry->Length);
    mPrimitivesWriter.PadTo4();
    endOfStream = mFontFileStream.GetCurrentPosition();

    // set the checksum to 0, and save its position for later update
    mHeadCheckSumOffset = startTableOffset + 8;
    mFontFileStream.SetPosition(mHeadCheckSumOffset);
    mPrimitivesWriter.WriteULONG(0);

    // set the loca format to longs
    mFontFileStream.SetPosition(startTableOffset + 50);
    mPrimitivesWriter.WriteUSHORT(1);

    // write table entry data, which includes movement
    WriteTableEntryData(mHEADEntryWritingOffset, startTableOffset, tableEntry->Length);

    // restore position to end of stream
    mFontFileStream.SetPosition(endOfStream);

    return mPrimitivesWriter.GetInternalState();
}

void TrueTypeEmbeddedFontWriter::WriteTableEntryData(long long inTableEntryOffset, long long inTableOffset,
                                                     unsigned long inTableLength)
{
    unsigned long checksum = GetCheckSum(inTableOffset, inTableLength);

    mFontFileStream.SetPosition(inTableEntryOffset);
    mPrimitivesWriter.WriteULONG(checksum);
    mPrimitivesWriter.WriteULONG((unsigned long)inTableOffset);
    mPrimitivesWriter.WriteULONG(inTableLength);
}

unsigned long TrueTypeEmbeddedFontWriter::GetCheckSum(long long inOffset, unsigned long inLength)
{
    unsigned long sum = 0L;
    auto endPosition = (unsigned long)(inOffset + ((inLength + 3) & ~3) / 4);
    auto position = (unsigned long)inOffset;
    unsigned long value;

    mFontFileStream.SetPosition(inOffset);

    while (position < endPosition)
    {
        mPrimitivesReader.ReadULONG(value);
        sum += value;
        position += 4;
    }
    return sum;
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteHHea()
{
    // copy as is, then possibly adjust the hmtx NumberOfHMetrics field, if the glyphs
    // count is lower

    TableEntry *tableEntry = mTrueTypeInput.GetTableEntry("hhea");
    long long startTableOffset;
    OutputStreamTraits streamCopier(&mFontFileStream);
    long long endOfStream;

    startTableOffset = mFontFileStream.GetCurrentPosition();

    // copy and save the current position
    mTrueTypeFile.GetInputStream()->SetPosition(tableEntry->Offset);
    streamCopier.CopyToOutputStream(mTrueTypeFile.GetInputStream(), tableEntry->Length);
    mPrimitivesWriter.PadTo4();
    endOfStream = mFontFileStream.GetCurrentPosition();

    // adjust the NumberOfHMetrics if necessary
    if (mTrueTypeInput.mHHea.NumberOfHMetrics > mSubsetFontGlyphsCount)
    {
        mFontFileStream.SetPosition(startTableOffset + tableEntry->Length - 2);
        mPrimitivesWriter.WriteUSHORT(mSubsetFontGlyphsCount);
    }

    // write table entry data, which includes movement
    WriteTableEntryData(mHHEAEntryWritingOffset, startTableOffset, tableEntry->Length);

    // restore position to end of stream
    mFontFileStream.SetPosition(endOfStream);

    return mPrimitivesWriter.GetInternalState();
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteHMtx()
{
    // k. basically i'm supposed to fill this up till the max glyph count.
    // so i'll just use the original table (keeping an eye on the NumberOfHMetrics)
    // filling with the original values (doesn't really matter for empty glyphs) till the
    // glyph count

    long long startTableOffset;

    startTableOffset = mFontFileStream.GetCurrentPosition();

    // write the table. write pairs until min(numberofhmetrics,mSubsetFontGlyphsCount)
    // then if mSubsetFontGlyphsCount > numberofhmetrics writh the width metrics as well
    unsigned numberOfHMetrics = std::min(mTrueTypeInput.mHHea.NumberOfHMetrics, mSubsetFontGlyphsCount);
    uint16_t i = 0;
    for (; i < numberOfHMetrics; ++i)
    {
        mPrimitivesWriter.WriteUSHORT(mTrueTypeInput.mHMtx[i].AdvanceWidth);
        mPrimitivesWriter.WriteSHORT(mTrueTypeInput.mHMtx[i].LeftSideBearing);
    }
    for (; i < mSubsetFontGlyphsCount; ++i)
        mPrimitivesWriter.WriteSHORT(mTrueTypeInput.mHMtx[i].LeftSideBearing);

    long long endOfTable = mFontFileStream.GetCurrentPosition();
    mPrimitivesWriter.PadTo4();
    long long endOfStream = mFontFileStream.GetCurrentPosition();

    // write table entry data, which includes movement
    WriteTableEntryData(mHMTXEntryWritingOffset, startTableOffset, (unsigned long)(endOfTable - startTableOffset));

    // restore position to end of stream
    mFontFileStream.SetPosition(endOfStream);

    return mPrimitivesWriter.GetInternalState();
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteMaxp()
{
    // copy as is, then adjust the glyphs count

    TableEntry *tableEntry = mTrueTypeInput.GetTableEntry("maxp");
    long long startTableOffset;
    OutputStreamTraits streamCopier(&mFontFileStream);
    long long endOfStream;

    startTableOffset = mFontFileStream.GetCurrentPosition();

    // copy and save the current position
    mTrueTypeFile.GetInputStream()->SetPosition(tableEntry->Offset);
    streamCopier.CopyToOutputStream(mTrueTypeFile.GetInputStream(), tableEntry->Length);
    mPrimitivesWriter.PadTo4();
    endOfStream = mFontFileStream.GetCurrentPosition();

    // adjust the glyphs count if necessary
    mFontFileStream.SetPosition(startTableOffset + 4);
    mPrimitivesWriter.WriteUSHORT(mSubsetFontGlyphsCount);

    // write table entry data, which includes movement
    WriteTableEntryData(mMAXPEntryWritingOffset, startTableOffset, tableEntry->Length);

    // restore position to end of stream
    mFontFileStream.SetPosition(endOfStream);

    return mPrimitivesWriter.GetInternalState();
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteCVT()
{
    return CreateTableCopy("cvt ", mCVTEntryWritingOffset);
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteFPGM()
{
    return CreateTableCopy("fpgm", mFPGMEntryWritingOffset);
}

EStatusCode TrueTypeEmbeddedFontWriter::WritePREP()
{
    return CreateTableCopy("prep", mPREPEntryWritingOffset);
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteGlyf(const UIntVector &inSubsetGlyphIDs, unsigned long *inLocaTable)
{
    // k. write the glyphs table. you only need to write the glyphs you are actually using.
    // while at it...update the locaTable

    TableEntry *tableEntry = mTrueTypeInput.GetTableEntry("glyf");
    long long startTableOffset = mFontFileStream.GetCurrentPosition();
    auto it = inSubsetGlyphIDs.begin();
    OutputStreamTraits streamCopier(&mFontFileStream);
    uint16_t glyphIndex, previousGlyphIndexEnd = 0;
    inLocaTable[0] = 0;
    EStatusCode status = eSuccess;

    for (; it != inSubsetGlyphIDs.end() && eSuccess == status; ++it)
    {
        glyphIndex = *it;
        if (glyphIndex >= mTrueTypeInput.mMaxp.NumGlyphs)
        {
            TRACE_LOG2("TrueTypeEmbeddedFontWriter::WriteGlyf, error, requested glyph index %ld is larger than the "
                       "maximum glyph index for this font which is %ld. ",
                       glyphIndex, mTrueTypeInput.mMaxp.NumGlyphs - 1);
            status = eFailure;
            break;
        }

        for (uint16_t i = previousGlyphIndexEnd + 1; i <= glyphIndex; ++i)
            inLocaTable[i] = inLocaTable[previousGlyphIndexEnd];
        if (mTrueTypeInput.mGlyf[glyphIndex] != nullptr)
        {
            mTrueTypeFile.GetInputStream()->SetPosition(tableEntry->Offset + mTrueTypeInput.mLoca[glyphIndex]);
            streamCopier.CopyToOutputStream(mTrueTypeFile.GetInputStream(),
                                            mTrueTypeInput.mLoca[(glyphIndex) + 1] - mTrueTypeInput.mLoca[glyphIndex]);
        }
        inLocaTable[glyphIndex + 1] = (unsigned long)(mFontFileStream.GetCurrentPosition() - startTableOffset);
        previousGlyphIndexEnd = glyphIndex + 1;
    }

    long long endOfTable = mFontFileStream.GetCurrentPosition();
    mPrimitivesWriter.PadTo4();
    long long endOfStream = mFontFileStream.GetCurrentPosition();

    // write table entry data, which includes movement
    WriteTableEntryData(mGLYFEntryWritingOffset, startTableOffset, (unsigned long)(endOfTable - startTableOffset));

    // restore position to end of stream
    mFontFileStream.SetPosition(endOfStream);

    return mPrimitivesWriter.GetInternalState();
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteLoca(unsigned long *inLocaTable)
{
    // k. just write the input locatable. in longs format

    long long startTableOffset = mFontFileStream.GetCurrentPosition();

    // write the table. write pairs until min(numberofhmetrics,mSubsetFontGlyphsCount)
    // then if mSubsetFontGlyphsCount > numberofhmetrics writh the width metrics as well
    for (uint16_t i = 0; i < mSubsetFontGlyphsCount + 1; ++i)
        mPrimitivesWriter.WriteULONG(inLocaTable[i]);

    long long endOfTable = mFontFileStream.GetCurrentPosition();
    mPrimitivesWriter.PadTo4();
    long long endOfStream = mFontFileStream.GetCurrentPosition();

    // write table entry data, which includes movement
    WriteTableEntryData(mLOCAEntryWritingOffset, startTableOffset, (unsigned long)(endOfTable - startTableOffset));

    // restore position to end of stream
    mFontFileStream.SetPosition(endOfStream);

    return mPrimitivesWriter.GetInternalState();
}

EStatusCode TrueTypeEmbeddedFontWriter::CreateHeadTableCheckSumAdjustment()
{
    long long endStream = mFontFileStream.GetCurrentPosition();
    unsigned long checkSum = 0xb1b0afba - GetCheckSum(0, (unsigned long)endStream);

    mFontFileStream.SetPosition(mHeadCheckSumOffset);
    mPrimitivesWriter.WriteULONG(checkSum);
    mFontFileStream.SetPosition(endStream); // restoring position just for kicks

    return mPrimitivesWriter.GetInternalState();
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteNAME()
{
    return CreateTableCopy("name", mNAMEEntryWritingOffset);
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteOS2()
{
    return CreateTableCopy("OS/2", mOS2EntryWritingOffset);
}

EStatusCode TrueTypeEmbeddedFontWriter::WriteCMAP()
{
    return CreateTableCopy("cmap", mCMAPEntryWritingOffset);
}

EStatusCode TrueTypeEmbeddedFontWriter::CreateTableCopy(const char *inTableName, long long inTableEntryLocation)
{
    // copy as is, no adjustments required

    TableEntry *tableEntry = mTrueTypeInput.GetTableEntry(inTableName);
    long long startTableOffset;
    OutputStreamTraits streamCopier(&mFontFileStream);
    long long endOfStream;

    startTableOffset = mFontFileStream.GetCurrentPosition();

    // copy and save the current position
    mTrueTypeFile.GetInputStream()->SetPosition(tableEntry->Offset);
    streamCopier.CopyToOutputStream(mTrueTypeFile.GetInputStream(), tableEntry->Length);
    mPrimitivesWriter.PadTo4();
    endOfStream = mFontFileStream.GetCurrentPosition();

    // write table entry data, which includes movement
    WriteTableEntryData(inTableEntryLocation, startTableOffset, tableEntry->Length);

    // restore position to end of stream
    mFontFileStream.SetPosition(endOfStream);

    return mPrimitivesWriter.GetInternalState();
}