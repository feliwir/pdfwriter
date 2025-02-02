/*
 Source File : PDFPageInput.h


 Copyright 2012 Gal Kahana PDFWriter

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

/*
    High level object for retrieving Page related info from a parsed page.
    pass a parser and a page object, and you'll be able to get boxes 'n such.

    ownership rules for the page object is like a standard PDFObjectCastPtr or
    RefPo
 */

#include "PDFRectangle.h"
#include "objects/PDFDictionary.h"
#include "objects/PDFObjectCast.h"

#include <string>

class PDFParser;
namespace charta
{
class PDFArray;
class PDFObject;

class PDFPageInput
{
  public:
    // simple constructor, not calling addref on inPageObject
    PDFPageInput(PDFParser *inParser, std::shared_ptr<PDFObject> inPageObject);
    // constructors from a smart pointer or another page object, will call addref
    PDFPageInput(PDFParser *inParser, const PDFObjectCastPtr<PDFDictionary> &inPageObject);
    PDFPageInput(const PDFPageInput &inOtherPage);

    // will call release on the input page object
    ~PDFPageInput();

    bool operator!();

    int GetRotate();
    PDFRectangle GetMediaBox();
    PDFRectangle GetCropBox();
    PDFRectangle GetTrimBox();
    PDFRectangle GetBleedBox();
    PDFRectangle GetArtBox();

  private:
    PDFParser *mParser;
    PDFObjectCastPtr<PDFDictionary> mPageObject;

    std::shared_ptr<PDFObject> QueryInheritedValue(const std::shared_ptr<PDFDictionary> &inDictionary,
                                                   const std::string &inName);
    void SetPDFRectangleFromPDFArray(const std::shared_ptr<PDFArray> &inPDFArray, PDFRectangle &outPDFRectangle);

    void AssertPageObjectValid();
    PDFRectangle GetBoxAndDefaultWithCrop(const std::string &inBoxName);
};
} // namespace charta