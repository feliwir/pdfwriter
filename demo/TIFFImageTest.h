/*
   Source File : TIFFImageTest.h


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

#ifndef PDFHUMMUS_NO_TIFF

#include "ITestUnit.h"

class PDFWriter;

class TIFFImageTest : public ITestUnit
{
  public:
    TIFFImageTest(void);
    virtual ~TIFFImageTest(void);

    virtual PDFHummus::EStatusCode Run(const TestConfiguration &inTestConfiguration);

  private:
    PDFHummus::EStatusCode AddPageForTIFF(PDFWriter &inPDFWriter, const string &inTiffFilePath);
};

#endif
