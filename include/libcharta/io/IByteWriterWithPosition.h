/*
   Source File : IByteWriterWithPosition.h


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
    IByteWriterWithPosition.
    extension of charta::IByteWriter that also allows for retrieving the current position

*/

#include "IByteWriter.h"

namespace charta
{
class IByteWriterWithPosition : public IByteWriter
{
  public:
    virtual ~IByteWriterWithPosition(){};
    virtual long long GetCurrentPosition() = 0;
};
} // namespace charta