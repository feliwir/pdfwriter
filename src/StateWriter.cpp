/*
   Source File : StateWriter.cpp


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
#include "StateWriter.h"
#include "DictionaryContext.h"
#include "ObjectsContext.h"
#include "Trace.h"
#include "io/IByteWriterWithPosition.h"

using namespace charta;

StateWriter::StateWriter()
{
    mObjectsContext = nullptr;
}

StateWriter::~StateWriter()
{
    delete mObjectsContext;
}

EStatusCode StateWriter::Start(const std::string &inStateFilePath)
{

    // open the new file...
    if (mOutputFile.OpenFile(inStateFilePath) != charta::eSuccess)
    {
        TRACE_LOG1("StateWriter::Start, can't open file for state writing in %s", inStateFilePath.c_str());
        return charta::eFailure;
    }

    // Get me a new copy of objects context, for this session
    delete mObjectsContext;
    mObjectsContext = new ObjectsContext();
    mObjectsContext->SetOutputStream(mOutputFile.GetOutputStream());

    // Put up a nice 'lil header comment. nice one, eh?
    mObjectsContext->WriteComment("PDFHummus-1.0");

    // Reset the root object
    mRootObject = 0;

    return charta::eSuccess;
}

ObjectsContext *StateWriter::GetObjectsWriter()
{
    return mObjectsContext;
}

EStatusCode StateWriter::Finish()
{
    EStatusCode status;
    do
    {
        long long xrefTablePosition;

        status = mObjectsContext->WriteXrefTable(xrefTablePosition);
        if (status != 0)
            break;

        WriteTrailerDictionary();
        WriteXrefReference(xrefTablePosition);
        WriteFinalEOF();

    } while (false);

    if (charta::eSuccess == status)
        status = mOutputFile.CloseFile();
    else
        mOutputFile.CloseFile();
    return status;
}

static const std::string scTrailer = "trailer";
static const std::string scSize = "Size";
static const std::string scRoot = "Root";
void StateWriter::WriteTrailerDictionary()
{
    DictionaryContext *dictionaryContext;

    mObjectsContext->WriteKeyword(scTrailer);
    dictionaryContext = mObjectsContext->StartDictionary();
    dictionaryContext->WriteKey(scSize);
    dictionaryContext->WriteIntegerValue(mObjectsContext->GetInDirectObjectsRegistry().GetObjectsCount());
    if (mRootObject != 0)
    {
        dictionaryContext->WriteKey(scRoot);
        dictionaryContext->WriteNewObjectReferenceValue(mRootObject);
    }
    mObjectsContext->EndDictionary(dictionaryContext);
}

void StateWriter::SetRootObject(ObjectIDType inRootObjectID)
{
    mRootObject = inRootObjectID;
}

static const std::string scStartXref = "startxref";
void StateWriter::WriteXrefReference(long long inXrefTablePosition)
{
    mObjectsContext->WriteKeyword(scStartXref);
    mObjectsContext->WriteInteger(inXrefTablePosition, eTokenSeparatorEndLine);
}

static const uint8_t scEOF[] = {'%', '%', 'E', 'O', 'F'};

void StateWriter::WriteFinalEOF()
{
    charta::IByteWriterWithPosition *freeContextOutput = mObjectsContext->StartFreeContext();
    freeContextOutput->Write(scEOF, 5);
    mObjectsContext->EndFreeContext();
}