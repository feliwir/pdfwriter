/*
   Source File : IByteReaderWithPosition.h


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
/*
    IByteReaderWithPositionWithPosition. extension of byte reader with position setting and getting
*/

#include "IByteReader.h"

namespace charta
{
class IByteReaderWithPosition : public IByteReader
{
  public:
    /*
        Set read position.always set from start
    */
    virtual void SetPosition(long long inOffsetFromStart) = 0;

    /*
        Set read position from end. measure is the number of bytes from end towards the beginning
    */
    virtual void SetPositionFromEnd(long long inOffsetFromEnd) = 0;

    /*
        Get the current read position
    */
    virtual long long GetCurrentPosition() = 0;

    /*
        skip position (like setting from current)
    */
    virtual void Skip(size_t inSkipSize) = 0;
};
} // namespace charta