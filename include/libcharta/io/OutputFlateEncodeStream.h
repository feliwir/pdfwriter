/*
   Source File : OutputFlateEncodeStream.h


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
#include "IByteWriterWithPosition.h"

struct z_stream_s;
typedef z_stream_s z_stream;

namespace charta
{

class OutputFlateEncodeStream final : public IByteWriterWithPosition
{
  public:
    OutputFlateEncodeStream();
    virtual ~OutputFlateEncodeStream();

    /* passing the target writer to OutputFlateEncodeStream makes it its owner. so if you don't want it to try and
     * delete it on destructor - use Assign(NULL) */
    OutputFlateEncodeStream(IByteWriterWithPosition *inTargetWriter, bool inInitiallyOn = true);

    // Assing makes OutputFlateEncodeStream the owner of inWriter, so if you don't want the class to delete it upon
    // destructions - use Assign(NULL)
    void Assign(IByteWriterWithPosition *inWriter, bool inInitiallyOn = true);

    virtual size_t Write(const uint8_t *inBuffer, size_t inSize);
    virtual long long GetCurrentPosition();

    void TurnOnEncoding();
    void TurnOffEncoding();

  private:
    uint8_t *mBuffer;
    IByteWriterWithPosition *mTargetStream;
    bool mCurrentlyEncoding;
    z_stream *mZLibState;

    void FinalizeEncoding();
    void StartEncoding();
    size_t EncodeBufferAndWrite(const uint8_t *inBuffer, size_t inSize);
};
} // namespace charta