/*
 Source File : BasicModification.cpp


 Copyright 2012 Gal Kahana PDFWriter

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
#include "PDFPage.h"
#include "PDFRectangle.h"
#include "PDFUsedFont.h"
#include "PDFWriter.h"
#include "PageContentContext.h"
#include "TestHelper.h"
#include "io/InputFile.h"
#include "io/OutputFile.h"
#include "io/OutputStreamTraits.h"

#include <gtest/gtest.h>
#include <iostream>

using namespace charta;

EStatusCode TestBasicFileModification(const std::string &inSourceFileName)
{
    PDFWriter pdfWriter;
    EStatusCode status = eSuccess;

    status = pdfWriter.ModifyPDF(
        RelativeURLToLocalPath(PDFWRITE_SOURCE_PATH, std::string("data/") + inSourceFileName + std::string(".pdf")),
        ePDFVersion13,
        RelativeURLToLocalPath(PDFWRITE_BINARY_PATH, std::string("Modified") + inSourceFileName + std::string(".pdf")),
        LogConfiguration(true, true,
                         RelativeURLToLocalPath(PDFWRITE_BINARY_PATH,
                                                std::string("Modified") + inSourceFileName + std::string(".log"))));
    if (status != charta::eSuccess)
        return status;

    PDFPage page;
    page.SetMediaBox(charta::PagePresets::A4_Portrait);

    PageContentContext *contentContext = pdfWriter.StartPageContentContext(page);
    if (nullptr == contentContext)
        return charta::eFailure;

    PDFUsedFont *font = pdfWriter.GetFontForFile(RelativeURLToLocalPath(PDFWRITE_SOURCE_PATH, "data/fonts/couri.ttf"));
    if (font == nullptr)
        return charta::eFailure;

    // Draw some text
    contentContext->BT();
    contentContext->k(0, 0, 0, 1);

    contentContext->Tf(font, 1);

    contentContext->Tm(30, 0, 0, 30, 78.4252, 662.8997);

    EStatusCode encodingStatus = contentContext->Tj("about");
    if (encodingStatus != charta::eSuccess)
        std::cout << "Could not find some of the glyphs for this font";

    // continue even if failed...want to see how it looks like
    contentContext->ET();

    status = pdfWriter.EndPageContentContext(contentContext);
    if (status != charta::eSuccess)
        return status;

    status = pdfWriter.WritePage(page);
    if (status != charta::eSuccess)
        return status;

    return pdfWriter.EndPDF();
}

EStatusCode TestInPlaceFileModification(const std::string &inSourceFileName)
{
    PDFWriter pdfWriter;
    EStatusCode status = eSuccess;
    // first copy source file to target
    {
        InputFile sourceFile;

        status = sourceFile.OpenFile(RelativeURLToLocalPath(
            PDFWRITE_SOURCE_PATH, std::string("data/") + inSourceFileName + std::string(".pdf")));
        if (status != charta::eSuccess)
            return status;

        OutputFile targetFile;

        status = targetFile.OpenFile(RelativeURLToLocalPath(
            PDFWRITE_BINARY_PATH, std::string("InPlaceModified") + inSourceFileName + std::string(".pdf")));
        if (status != charta::eSuccess)
            return status;

        OutputStreamTraits traits(targetFile.GetOutputStream());
        status = traits.CopyToOutputStream(sourceFile.GetInputStream());
        if (status != charta::eSuccess)
            return status;

        sourceFile.CloseFile();
        targetFile.CloseFile();
    }

    // now modify in place

    status = pdfWriter.ModifyPDF(
        RelativeURLToLocalPath(PDFWRITE_BINARY_PATH,
                               std::string("InPlaceModified") + inSourceFileName + std::string(".pdf")),
        ePDFVersion13, "",
        LogConfiguration(true, true,
                         RelativeURLToLocalPath(PDFWRITE_BINARY_PATH, std::string("InPlaceModified") +
                                                                          inSourceFileName + std::string(".log"))));
    if (status != charta::eSuccess)
        return status;

    PDFPage page;
    page.SetMediaBox(charta::PagePresets::A4_Portrait);

    PageContentContext *contentContext = pdfWriter.StartPageContentContext(page);
    if (contentContext == nullptr)
        return charta::eFailure;

    PDFUsedFont *font = pdfWriter.GetFontForFile(RelativeURLToLocalPath(PDFWRITE_SOURCE_PATH, "data/fonts/couri.ttf"));
    if (font == nullptr)
        return charta::eFailure;

    // Draw some text
    contentContext->BT();
    contentContext->k(0, 0, 0, 1);

    contentContext->Tf(font, 1);

    contentContext->Tm(30, 0, 0, 30, 78.4252, 662.8997);

    EStatusCode encodingStatus = contentContext->Tj("about");
    if (encodingStatus != charta::eSuccess)
        std::cout << "Could not find some of the glyphs for this font";

    // continue even if failed...want to see how it looks like
    contentContext->ET();

    status = pdfWriter.EndPageContentContext(contentContext);
    if (status != charta::eSuccess)
        return status;

    status = pdfWriter.WritePage(page);
    if (status != charta::eSuccess)
        return status;

    status = pdfWriter.EndPDF();

    return status;
}

TEST(Modification, BasicModification)
{
    // modification of different source and target
    ASSERT_EQ(TestBasicFileModification("BasicTIFFImagesTest"), eSuccess);
    ASSERT_EQ(TestBasicFileModification("Linearized"), eSuccess);
    ASSERT_EQ(TestBasicFileModification("MultipleChange"), eSuccess);
    ASSERT_EQ(TestBasicFileModification("RemovedItem"), eSuccess);
    ASSERT_NE(TestBasicFileModification("Protected"), eSuccess);
    ASSERT_EQ(TestBasicFileModification("ObjectStreams"), eSuccess);
    ASSERT_EQ(TestBasicFileModification("ObjectStreamsModified"), eSuccess);

    // in place modification
    ASSERT_EQ(TestInPlaceFileModification("BasicTIFFImagesTest"), eSuccess);
    ASSERT_EQ(TestInPlaceFileModification("Linearized"), eSuccess);
    ASSERT_EQ(TestInPlaceFileModification("MultipleChange"), eSuccess);
    ASSERT_EQ(TestInPlaceFileModification("RemovedItem"), eSuccess);
    ASSERT_EQ(TestInPlaceFileModification("ObjectStreams"), eSuccess);
    ASSERT_EQ(TestInPlaceFileModification("ObjectStreamsModified"), eSuccess);
}