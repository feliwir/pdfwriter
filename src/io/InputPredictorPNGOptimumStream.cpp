/*
   Source File : InputPredictorPNGOptimumStream.cpp


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
#include "io/InputPredictorPNGOptimumStream.h"

#include "Trace.h"
#include <stdlib.h>

/*
    Note from Gal: Note that optimum also implements the others. this is because PNG compression requires that the first
   byte in the line holds the algo - even if the whole stream is declared as a single algo.
*/

charta::InputPredictorPNGOptimumStream::InputPredictorPNGOptimumStream()
{
    mSourceStream = nullptr;
    mBuffer = nullptr;
    mIndex = nullptr;
    mBufferSize = 0;
    mUpValues = nullptr;
}

charta::InputPredictorPNGOptimumStream::~InputPredictorPNGOptimumStream()
{
    delete[] mBuffer;
    delete[] mUpValues;
    delete mSourceStream;
}

charta::InputPredictorPNGOptimumStream::InputPredictorPNGOptimumStream(charta::IByteReader *inSourceStream,
                                                                       size_t inColors, uint8_t inBitsPerComponent,
                                                                       size_t inColumns)
{
    mSourceStream = nullptr;
    mBuffer = nullptr;
    mIndex = nullptr;
    mBufferSize = 0;
    mUpValues = nullptr;

    Assign(inSourceStream, inColors, inBitsPerComponent, inColumns);
}

size_t charta::InputPredictorPNGOptimumStream::Read(uint8_t *inBuffer, size_t inBufferSize)
{
    size_t readBytes = 0;

    // exhaust what's in the buffer currently
    while (mBufferSize > (size_t)(mIndex - mBuffer) && readBytes < inBufferSize)
    {
        DecodeNextByte(inBuffer[readBytes]);
        ++readBytes;
    }

    // now repeatedly read bytes from the input stream, and decode
    while (readBytes < inBufferSize && mSourceStream->NotEnded())
    {
        memcpy(mUpValues, mBuffer, mBufferSize);
        size_t readFromSource = mSourceStream->Read(mBuffer, mBufferSize);
        if (readFromSource == 0)
        {
            break; // a belated end. must be flate
        }
        if (readFromSource != mBufferSize)
        {
            TRACE_LOG(
                "charta::InputPredictorPNGOptimumStream::Read, problem, expected columns number read. didn't make it");
            break;
        }
        mFunctionType = *mBuffer;
        *mBuffer = 0;         // so i can use this as "left" value...we don't care about this one...it's just a tag
        mIndex = mBuffer + 1; // skip the first tag

        while (mBufferSize > (size_t)(mIndex - mBuffer) && readBytes < inBufferSize)
        {
            DecodeNextByte(inBuffer[readBytes]);
            ++readBytes;
        }
    }
    return readBytes;
}

bool charta::InputPredictorPNGOptimumStream::NotEnded()
{
    return mSourceStream->NotEnded() || (size_t)(mIndex - mBuffer) < mBufferSize;
}

void charta::InputPredictorPNGOptimumStream::DecodeNextByte(uint8_t &outDecodedByte)
{
    // decoding function is determined by mFunctionType
    switch (mFunctionType)
    {
    case 0:
        outDecodedByte = *mIndex;
        break;
    case 1:
        outDecodedByte = (uint8_t)((char)*(mIndex - mBytesPerPixel) + (char)*mIndex);
        break;
    case 2:
        outDecodedByte = (uint8_t)((char)mUpValues[mIndex - mBuffer] + (char)*mIndex);
        break;
    case 3:
        outDecodedByte =
            (uint8_t)((char)mBuffer[mIndex - mBuffer - 1] / 2 + (char)mUpValues[mIndex - mBuffer] / 2 + (char)*mIndex);
        break;
    case 4:
        outDecodedByte = (uint8_t)(PaethPredictor(mBuffer[mIndex - mBuffer - 1], mUpValues[mIndex - mBuffer],
                                                  mUpValues[mIndex - mBuffer - 1]) +
                                   (char)*mIndex);
        break;
    }

    *mIndex = outDecodedByte; // saving the encoded value back to the buffer, for later copying as "Up value", and
                              // current using as "Left" value
    ++mIndex;
}

void charta::InputPredictorPNGOptimumStream::Assign(charta::IByteReader *inSourceStream, size_t inColors,
                                                    uint8_t inBitsPerComponent, size_t inColumns)
{
    mSourceStream = inSourceStream;

    delete[] mBuffer;
    delete[] mUpValues;
    mBytesPerPixel = inColors * inBitsPerComponent / 8;
    // Rows may contain empty bits at end
    mBufferSize = (inColumns * inColors * inBitsPerComponent + 7) / 8 + 1;
    mBuffer = new uint8_t[mBufferSize];
    memset(mBuffer, 0, mBufferSize);
    mUpValues = new uint8_t[mBufferSize];
    memset(mUpValues, 0, mBufferSize); // that's less important
    mIndex = mBuffer + mBufferSize;
    mFunctionType = 0;
}

char charta::InputPredictorPNGOptimumStream::PaethPredictor(char inLeft, char inUp, char inUpLeft)
{
    int p = inLeft + inUp - inUpLeft;
    int pLeft = abs(p - inLeft);
    int pUp = abs(p - inUp);
    int pUpLeft = abs(p - inUpLeft);

    if (pLeft <= pUp && pLeft <= pUpLeft)
        return pLeft;
    if (pUp <= pUpLeft)
        return inUp;
    return inUpLeft;
}
