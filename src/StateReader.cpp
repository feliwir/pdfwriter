/*
   Source File : StateReader.cpp


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
#include "StateReader.h"
#include "Trace.h"
#include "objects/PDFIndirectObjectReference.h"
#include "objects/PDFObjectCast.h"

using namespace charta;

StateReader::StateReader() = default;

StateReader::~StateReader() = default;

EStatusCode StateReader::Start(const std::string &inStateFilePath)
{
    // open the new file...
    if (mInputFile.OpenFile(inStateFilePath) != charta::eSuccess)
    {
        TRACE_LOG1("StateReader::Start, can't open file for state reading in %s", inStateFilePath.c_str());
        return charta::eFailure;
    }

    if (mParser.StartStateFileParsing(mInputFile.GetInputStream()) != charta::eSuccess)
    {
        TRACE_LOG("StateReader::Start, unable to start parsing for the state reader file");
        return charta::eFailure;
    }

    // set the root object
    PDFObjectCastPtr<charta::PDFIndirectObjectReference> rootObject(mParser.GetTrailer()->QueryDirectObject("Root"));
    mRootObject = rootObject->mObjectID;

    return charta::eSuccess;
}

PDFParser *StateReader::GetObjectsReader()
{
    return &mParser;
}

ObjectIDType StateReader::GetRootObjectID() const
{
    return mRootObject;
}

void StateReader::Finish()
{
    mParser.ResetParser();
}
