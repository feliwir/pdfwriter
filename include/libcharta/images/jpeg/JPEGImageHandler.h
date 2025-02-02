/*
   Source File : JPEGImageHandler.h


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

#include "JPEGImageInformation.h"
#include "ObjectsBasicTypes.h"

#include <map>
#include <set>
#include <string>
#include <utility>

class ObjectsContext;
class PDFImageXObject;
class IDocumentContextExtender;
class PDFFormXObject;

namespace charta
{
class IByteReaderWithPosition;
class DocumentContext;

using StringToJPEGImageInformationMap = std::map<std::string, JPEGImageInformation>;
using BoolAndJPEGImageInformation = std::pair<bool, JPEGImageInformation>;
using IDocumentContextExtenderSet = std::set<IDocumentContextExtender *>;

class JPEGImageHandler
{
  public:
    JPEGImageHandler();

    // use this for retrieving image information for JPEG (useful for deciphering JPG dimensions tags)
    BoolAndJPEGImageInformation RetrieveImageInformation(const std::string &inJPGFilePath);
    BoolAndJPEGImageInformation RetrieveImageInformation(IByteReaderWithPosition *inJPGStream);

    // DocumentContext::CreateImageXObjectFromJPGFile are equivelent
    PDFImageXObject *CreateImageXObjectFromJPGFile(const std::string &inJPGFilePath);
    PDFImageXObject *CreateImageXObjectFromJPGStream(IByteReaderWithPosition *inJPGStream);
    PDFImageXObject *CreateImageXObjectFromJPGFile(const std::string &inJPGFilePath, ObjectIDType inImageXObjectID);
    PDFImageXObject *CreateImageXObjectFromJPGStream(IByteReaderWithPosition *inJPGStream,
                                                     ObjectIDType inImageXObjectID);

    // will return form XObject, which will include the xobject at it's size
    PDFFormXObject *CreateFormXObjectFromJPGFile(const std::string &inJPGFilePath);
    PDFFormXObject *CreateFormXObjectFromJPGStream(IByteReaderWithPosition *inJPGStream);
    PDFFormXObject *CreateFormXObjectFromJPGFile(const std::string &inJPGFilePath, ObjectIDType inFormXObjectID);
    PDFFormXObject *CreateFormXObjectFromJPGStream(IByteReaderWithPosition *inJPGStream, ObjectIDType inFormXObjectID);

    void SetOperationsContexts(DocumentContext *inDocumentContext, ObjectsContext *inObjectsContext);
    void AddDocumentContextExtender(IDocumentContextExtender *inExtender);
    void RemoveDocumentContextExtender(IDocumentContextExtender *inExtender);

    void Reset();

    // use the top RetrieveImageInformation to get a JPEGImageInformation, and then get to here to get the dimensions
    // that PDFHummus will use if asked to place "as is"
    std::pair<double, double> GetImageDimensions(const JPEGImageInformation &inJPGImageInformation);
    int GetColorComponents(const JPEGImageInformation &inJPGImageInformation);

  private:
    JPEGImageInformation mNullInformation;
    StringToJPEGImageInformationMap mImagesInformationMap;
    ObjectsContext *mObjectsContext;
    DocumentContext *mDocumentContext;
    IDocumentContextExtenderSet mExtenders;

    PDFImageXObject *CreateAndWriteImageXObjectFromJPGInformation(const std::string &inJPGFilePath,
                                                                  ObjectIDType inImageXObjectID,
                                                                  const JPEGImageInformation &inJPGImageInformation);
    PDFImageXObject *CreateAndWriteImageXObjectFromJPGInformation(charta::IByteReaderWithPosition *inJPGImageStream,
                                                                  ObjectIDType inImageXObjectID,
                                                                  const JPEGImageInformation &inJPGImageInformation);
    PDFFormXObject *CreateImageFormXObjectFromImageXObject(PDFImageXObject *inImageXObject,
                                                           ObjectIDType inFormXObjectID,
                                                           const JPEGImageInformation &inJPGImageInformation);
};
} // namespace charta