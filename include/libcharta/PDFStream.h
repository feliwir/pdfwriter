/*
   Source File : PDFStream.h


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
    PDFStream objects represents a stream in the PDF.
    due to @#$@$ Length key in stream dictionary, stream writing in the library is a two step matter.
    first use PDFStream to write the stream content, then write the Stream to the PDF using ObjectContext WriteStream
*/

#include "EStatusCode.h"
#include "MyStringBuf.h"
#include "ObjectsBasicTypes.h"
#include "io/OutputFlateEncodeStream.h"
#include "io/OutputStringBufferStream.h"
#include <sstream>
#include <stdint.h>
#include <stdio.h>

namespace charta
{
class IByteWriterWithPosition;
}
class IObjectsContextExtender;
class DictionaryContext;
class EncryptionHelper;

class PDFStream
{
  public:
    PDFStream(bool inCompressStream, charta::IByteWriterWithPosition *inOutputStream,
              EncryptionHelper *inEncryptionHelper, ObjectIDType inExtentObjectID,
              IObjectsContextExtender *inObjectsContextExtender);

    PDFStream(bool inCompressStream, charta::IByteWriterWithPosition *inOutputStream,
              EncryptionHelper *inEncryptionHelper, DictionaryContext *inStreamDictionaryContextForDirectExtentStream,
              IObjectsContextExtender *inObjectsContextExtender);

    ~PDFStream();

    // Get the output stream of the PDFStream, make sure to use only before calling FinalizeStreamWrite, after which it
    // becomes invalid
    charta::IByteWriter *GetWriteStream();

    // when done with writing to the stream call FinalizeWriteStream to get all writing resources released and calculate
    // the stream extent. For streams where extent writing is direct object, there is still a call needed later, to
    // FlushStreamContentForDirectExtentStream() to actually write it.
    void FinalizeStreamWrite();

    bool IsStreamCompressed() const;
    ObjectIDType GetExtentObjectID() const;

    long long GetLength() const; // get the stream extent

    // direct extent specific
    DictionaryContext *GetStreamDictionaryForDirectExtentStream();
    void FlushStreamContentForDirectExtentStream();

  private:
    bool mCompressStream;
    charta::OutputFlateEncodeStream mFlateEncodingStream;
    charta::IByteWriterWithPosition *mOutputStream;
    charta::IByteWriterWithPosition *mEncryptionStream;
    ObjectIDType mExtendObjectID;
    long long mStreamLength;
    long long mStreamStartPosition;
    charta::IByteWriter *mWriteStream;
    IObjectsContextExtender *mExtender;
    MyStringBuf mTemporaryStream;
    charta::OutputStringBufferStream mTemporaryOutputStream;
    DictionaryContext *mStreamDictionaryContextForDirectExtentStream;
};
