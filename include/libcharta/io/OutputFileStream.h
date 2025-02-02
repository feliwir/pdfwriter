/*
   Source File : OutputFileStream.h


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

#include "EStatusCode.h"
#include "IByteWriterWithPosition.h"

#include <fstream>

namespace charta
{
class OutputFileStream final : public IByteWriterWithPosition
{
  public:
    OutputFileStream() = default;
    virtual ~OutputFileStream();

    // input file path is in UTF8
    OutputFileStream(const std::string &inFilePath, bool inAppend = false);

    // input file path is in UTF8
    charta::EStatusCode Open(const std::string &inFilePath, bool inAppend = false);
    charta::EStatusCode Close();

    // charta::IByteWriter implementation
    virtual size_t Write(const uint8_t *inBuffer, size_t inSize);

    // IByteWriterWithPosition implementation
    virtual long long GetCurrentPosition();

  private:
    std::ofstream mStream;
};
} // namespace charta