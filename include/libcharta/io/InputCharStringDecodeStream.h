/*
   Source File : InputCharStringDecodeStream.h


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
#include "IByteReader.h"

class InputCharStringDecodeStream final : public charta::IByteReader
{
  public:
    InputCharStringDecodeStream(charta::IByteReader *inReadFrom, unsigned long inLenIV = 4);
    ~InputCharStringDecodeStream(void);

    void Assign(charta::IByteReader *inReadFrom, unsigned long inLenIV = 4);

    // IByteReader implementation

    virtual size_t Read(uint8_t *inBuffer, size_t inBufferSize);
    virtual bool NotEnded();

  private:
    charta::IByteReader *mReadFrom;
    uint16_t mRandomizer;

    void InitializeCharStringDecode(unsigned long inLenIV);
    charta::EStatusCode ReadDecodedByte(uint8_t &outByte);
    uint8_t DecodeByte(uint8_t inByteToDecode);
};
