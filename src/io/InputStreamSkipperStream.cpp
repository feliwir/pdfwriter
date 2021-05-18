/*
   Source File : InputStreamSkipperStream.cpp


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
#include "InputStreamSkipperStream.h"

InputStreamSkipperStream::InputStreamSkipperStream()
{
    mStream = nullptr;
}

InputStreamSkipperStream::~InputStreamSkipperStream()
{
    if (mStream != nullptr)
        delete mStream;
}

InputStreamSkipperStream::InputStreamSkipperStream(IByteReader *inSourceStream)
{
    Assign(inSourceStream);
}

void InputStreamSkipperStream::Assign(IByteReader *inSourceStream)
{
    mStream = inSourceStream;
    mAmountRead = 0;
}

size_t InputStreamSkipperStream::Read(uint8_t *inBuffer, size_t inBufferSize)
{
    size_t readThisTime = mStream->Read(inBuffer, inBufferSize);
    mAmountRead += readThisTime;

    return readThisTime;
}

bool InputStreamSkipperStream::NotEnded()
{
    return mStream != nullptr ? mStream->NotEnded() : false;
}

bool InputStreamSkipperStream::CanSkipTo(long long inPositionInStream) const
{
    return mAmountRead <= inPositionInStream;
}

void InputStreamSkipperStream::SkipTo(long long inPositionInStream)
{
    if (!CanSkipTo(inPositionInStream))
        return;

    SkipBy(inPositionInStream - mAmountRead);
}

// will skip by, or hit EOF
void InputStreamSkipperStream::SkipBy(long long inAmountToSkipBy)
{
    uint8_t buffer;

    while (NotEnded() && inAmountToSkipBy > 0)
    {
        Read(&buffer, 1);
        --inAmountToSkipBy;
    }
}

void InputStreamSkipperStream::Reset()
{
    mAmountRead = 0;
}

long long InputStreamSkipperStream::GetCurrentPosition()
{
    return mAmountRead;
}