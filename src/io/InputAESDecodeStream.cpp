/*
Source File : InputAESDecodeStream.h


Copyright 2016 Gal Kahana PDFWriter

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

#include "io/InputAESDecodeStream.h"

#include <algorithm>
#include <string.h>

charta::InputAESDecodeStream::InputAESDecodeStream()
{
    mSourceStream = nullptr;
}

charta::InputAESDecodeStream::InputAESDecodeStream(IByteReader *inSourceReader, const ByteList &inKey)
{
    Assign(inSourceReader, inKey);
}

charta::InputAESDecodeStream::~InputAESDecodeStream()
{
    if (mSourceStream != nullptr)
        delete mSourceStream;

    if (mKey != nullptr)
        delete[] mKey;
}

void charta::InputAESDecodeStream::Assign(IByteReader *inSourceReader, const ByteList &inKey)
{
    mSourceStream = inSourceReader;

    // convert inEncryptionKey to internal rep and init decrypt [let's hope its 16...]
    mKeyLength = inKey.size();
    mKey = new unsigned char[mKeyLength];
    auto it = inKey.begin();
    size_t i = 0;
    for (; it != inKey.end(); ++i, ++it)
        mKey[i] = *it;
    mDecrypt.key(mKey, mKeyLength);
    mIsIvInit = false; // first read flag. still need to read IV
    mReadBlockSize = AES_BLOCK_SIZE;
    mOutIndex = mOut + mReadBlockSize;
    mHitEnd = false;
}

bool charta::InputAESDecodeStream::NotEnded()
{
    return ((mSourceStream != nullptr) && mSourceStream->NotEnded()) || !mHitEnd ||
           ((mOutIndex - mOut) < mReadBlockSize);
}

size_t charta::InputAESDecodeStream::Read(uint8_t *inBuffer, size_t inSize)
{
    if (mSourceStream == nullptr)
        return 0;

    // if iv not init yet, init now
    if (!mIsIvInit)
    {
        // read iv buffer
        size_t ivRead = mSourceStream->Read(mIV, AES_BLOCK_SIZE);
        if (ivRead < AES_BLOCK_SIZE)
            return 0;
        // read first buffer
        size_t firstBlockLength = mSourceStream->Read(mInNext, AES_BLOCK_SIZE);
        if (firstBlockLength < AES_BLOCK_SIZE)
            return 0;

        // decrypt first buffer
        memcpy(mIn, mInNext, AES_BLOCK_SIZE);
        if (mDecrypt.cbc_decrypt(mIn, mOut, AES_BLOCK_SIZE, mIV) != EXIT_SUCCESS)
            return 0;
        mOutIndex = mOut;

        // read next buffer, to determine if first buffer contains padding
        size_t secondBlockLength = mSourceStream->Read(mInNext, AES_BLOCK_SIZE);
        if (secondBlockLength < AES_BLOCK_SIZE)
        {
            mHitEnd = true;
            // secondBlockLength should be 0. this is the case that first buffer already contains padding (using min for
            // safety)
            mReadBlockSize = AES_BLOCK_SIZE - std::min<size_t>(mOut[AES_BLOCK_SIZE - 1], AES_BLOCK_SIZE);
        }
        else
            mReadBlockSize = AES_BLOCK_SIZE;
        mIsIvInit = true;
    }

    size_t left = inSize;
    size_t remainderRead;

    while (left > 0)
    {
        remainderRead = std::min<size_t>(left, (mReadBlockSize - (mOutIndex - mOut)));
        if (remainderRead > 0)
        {
            // fill block with remainder from latest decryption
            memcpy(inBuffer + inSize - left, mOutIndex, remainderRead);
            mOutIndex += remainderRead;
            left -= remainderRead;
        }

        if (left > 0)
        {
            if (mHitEnd)
            {
                // that's true EOF...so finish
                break;
            }

            // decrypt next block
            memcpy(mIn, mInNext, AES_BLOCK_SIZE);
            if (mDecrypt.cbc_decrypt(mIn, mOut, AES_BLOCK_SIZE, mIV) != EXIT_SUCCESS)
                break;
            mOutIndex = mOut;

            // read next buffer from input stream
            size_t totalRead = mSourceStream->Read(mInNext, AES_BLOCK_SIZE);
            if (totalRead < AES_BLOCK_SIZE)
            {                   // this means that we got to final block, with padding
                mHitEnd = true; // should be 0.
                // now we know that next block is the final one, and can consider padding (using min for safety)
                mReadBlockSize = AES_BLOCK_SIZE - std::min<size_t>(mOut[AES_BLOCK_SIZE - 1], AES_BLOCK_SIZE);
                // Gal: one can find out here that the next block is actually empty...that's not gonna be great for
                // NotEnded + read of 1 byte....
            }
        }
    }

    return inSize - left;
}