/*
   Source File : PageContentContext.cpp


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
#include "PageContentContext.h"
#include "DocumentContext.h"
#include "IPageEndWritingTask.h"
#include "ObjectsContext.h"
#include "PDFPage.h"
#include "PDFStream.h"
#include "Trace.h"

using namespace charta;

PageContentContext::PageContentContext(charta::DocumentContext *inDocumentContext, PDFPage &inPageOfContext,
                                       ObjectsContext *inObjectsContext)
    : AbstractContentContext(inDocumentContext), mPageOfContext(inPageOfContext)
{
    mObjectsContext = inObjectsContext;
    mCurrentStream = nullptr;
}

PageContentContext::~PageContentContext() = default;

void PageContentContext::StartAStreamIfRequired()
{
    if (mCurrentStream == nullptr)
    {
        StartContentStreamDefinition();
        mCurrentStream = mObjectsContext->StartPDFStream();
        SetPDFStreamForWrite(mCurrentStream);
    }
}

void PageContentContext::StartContentStreamDefinition()
{
    ObjectIDType streamObjectID = mObjectsContext->StartNewIndirectObject();
    mPageOfContext.AddContentStreamReference(streamObjectID);
}

ResourcesDictionary *PageContentContext::GetResourcesDictionary()
{
    return &(mPageOfContext.GetResourcesDictionary());
}

EStatusCode PageContentContext::FinalizeCurrentStream()
{
    if (mCurrentStream != nullptr)
        return FinalizeStreamWriteAndRelease();
    return charta::eSuccess;
}

PDFPage &PageContentContext::GetAssociatedPage()
{
    return mPageOfContext;
}

EStatusCode PageContentContext::FinalizeStreamWriteAndRelease()
{
    mObjectsContext->EndPDFStream(mCurrentStream);
    mCurrentStream = nullptr;
    return charta::eSuccess;
}

std::shared_ptr<PDFStream> PageContentContext::GetCurrentPageContentStream()
{
    StartAStreamIfRequired();
    return mCurrentStream;
}

void PageContentContext::RenewStreamConnection()
{
    StartAStreamIfRequired();
}

class PageImageWritingTask : public IPageEndWritingTask
{
  public:
    PageImageWritingTask(const std::string &inImagePath, unsigned long inImageIndex, ObjectIDType inObjectID,
                         const PDFParsingOptions &inPDFParsingOptions)
    {
        mImagePath = inImagePath;
        mImageIndex = inImageIndex;
        mObjectID = inObjectID;
        mPDFParsingOptions = inPDFParsingOptions;
    }

    ~PageImageWritingTask() override = default;

    charta::EStatusCode Write(PDFPage & /*inPageObject*/, ObjectsContext * /*inObjectsContext*/,
                              charta::DocumentContext *inDocumentContext) override
    {
        return inDocumentContext->WriteFormForImage(mImagePath, mImageIndex, mObjectID, mPDFParsingOptions);
    }

  private:
    std::string mImagePath;
    unsigned long mImageIndex;
    ObjectIDType mObjectID;
    PDFParsingOptions mPDFParsingOptions;
};

void PageContentContext::ScheduleImageWrite(const std::string &inImagePath, unsigned long inImageIndex,
                                            ObjectIDType inObjectID, const PDFParsingOptions &inParsingOptions)
{
    mDocumentContext->RegisterPageEndWritingTask(
        GetAssociatedPage(), new PageImageWritingTask(inImagePath, inImageIndex, inObjectID, inParsingOptions));
}