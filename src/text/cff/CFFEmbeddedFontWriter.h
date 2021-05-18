/*
   Source File : CFFEmbeddedFontWriter.h


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
#pragma once

#include "CFFPrimitiveWriter.h"
#include "EStatusCode.h"
#include "MyStringBuf.h"
#include "ObjectsBasicTypes.h"
#include "io/InputFile.h"
#include "io/OutputStringBufferStream.h"
#include "text/opentype/OpenTypeFileInput.h"
#include <stdint.h>
#include <stdio.h>

#include <map>
#include <set>
#include <string>
#include <vector>

typedef std::vector<unsigned int> UIntVector;
typedef std::set<unsigned int> UIntSet;
typedef std::vector<unsigned short> UShortVector;
typedef std::map<FontDictInfo *, uint8_t> FontDictInfoToByteMap;

class FreeTypeFaceWrapper;
class ObjectsContext;

class CFFEmbeddedFontWriter
{
  public:
    CFFEmbeddedFontWriter(void);
    ~CFFEmbeddedFontWriter(void);

    /*
     *	Gal 26/8/2017. Important. The font created by this code is a subset of the original CFF code.
     *	The code recreates a subset of the font where the glyphs are ordered from 0 to inSubsetGlyphIDs.size().
     *	It does NOT retain the GID codes of the original font! as such, when using this embedded version
     *	and Identity-Y/V the codes used should be 0 to inSubsetGlyphIDs.size().
     *	This is unrelated to the note below - the IDs still keep their attachment to the original font glyph names
     *	and single character encoding, per the internal charset and encoding vectors.
     */

    PDFHummus::EStatusCode WriteEmbeddedFont(FreeTypeFaceWrapper &inFontInfo, const UIntVector &inSubsetGlyphIDs,
                                             const std::string &inFontFile3SubType, const std::string &inSubsetFontName,
                                             ObjectsContext *inObjectsContext, ObjectIDType &outEmbeddedFontObjectID);

    // the optional inCIDMapping parameter provides a vector ordered in the same
    // way as the glyph IDs. for each position in the CID mapping vector there's the matching CID
    // for the GID in the same position in the subset glyph IDs.
    // use it when the CFF origin is from a subset font, and the GID->CID mapping is not simply
    // identity
    PDFHummus::EStatusCode WriteEmbeddedFont(FreeTypeFaceWrapper &inFontInfo, const UIntVector &inSubsetGlyphIDs,
                                             const std::string &inFontFile3SubType, const std::string &inSubsetFontName,
                                             ObjectsContext *inObjectsContext, UShortVector *inCIDMapping,
                                             ObjectIDType &outEmbeddedFontObjectID);

  private:
    OpenTypeFileInput mOpenTypeInput;
    InputFile mOpenTypeFile;
    CFFPrimitiveWriter mPrimitivesWriter;
    OutputStringBufferStream mFontFileStream;
    bool mIsCID;
    std::string mOptionalEmbeddedPostscript;

    // placeholders positions
    long long mCharsetPlaceHolderPosition;
    long long mEncodingPlaceHolderPosition;
    long long mCharstringsPlaceHolderPosition;
    long long mPrivatePlaceHolderPosition;
    long long mFDArrayPlaceHolderPosition;
    long long mFDSelectPlaceHolderPosition;

    long long mEncodingPosition;
    long long mCharsetPosition;
    long long mCharStringPosition;
    long long mPrivatePosition;
    long long mPrivateSize;
    long long mFDArrayPosition;
    long long mFDSelectPosition;

    PDFHummus::EStatusCode CreateCFFSubset(FreeTypeFaceWrapper &inFontInfo, const UIntVector &inSubsetGlyphIDs,
                                           UShortVector *inCIDMapping, const std::string &inSubsetFontName,
                                           bool &outNotEmbedded, MyStringBuf &outFontProgram);
    PDFHummus::EStatusCode AddDependentGlyphs(UIntVector &ioSubsetGlyphIDs);
    PDFHummus::EStatusCode AddComponentGlyphs(unsigned int inGlyphID, UIntSet &ioComponents, bool &outFoundComponents);
    PDFHummus::EStatusCode WriteCFFHeader();
    PDFHummus::EStatusCode WriteName(const std::string &inSubsetFontName);
    PDFHummus::EStatusCode WriteTopIndex();
    uint8_t GetMostCompressedOffsetSize(unsigned long inOffset);
    PDFHummus::EStatusCode WriteTopDictSegment(MyStringBuf &ioTopDictSegment);
    PDFHummus::EStatusCode WriteStringIndex();
    PDFHummus::EStatusCode WriteGlobalSubrsIndex();
    PDFHummus::EStatusCode WriteEncodings(const UIntVector &inSubsetGlyphIDs);
    PDFHummus::EStatusCode WriteCharsets(const UIntVector &inSubsetGlyphIDs, UShortVector *inCIDMapping);
    PDFHummus::EStatusCode WriteCharStrings(const UIntVector &inSubsetGlyphIDs);
    PDFHummus::EStatusCode WritePrivateDictionary();

    PDFHummus::EStatusCode WriteFDArray(const UIntVector &inSubsetGlyphIDs,
                                        const FontDictInfoToByteMap &inNewFontDictsIndexes);
    PDFHummus::EStatusCode WriteFDSelect(const UIntVector &inSubsetGlyphIDs,
                                         const FontDictInfoToByteMap &inNewFontDictsIndexes);
    PDFHummus::EStatusCode WritePrivateDictionaryBody(const PrivateDictInfo &inPrivateDictionary,
                                                      long long &outWriteSize, long long &outWritePosition);
    PDFHummus::EStatusCode UpdateIndexesAtTopDict();

    void DetermineFDArrayIndexes(const UIntVector &inSubsetGlyphIDs, FontDictInfoToByteMap &outNewFontDictsIndexes) const;
};
