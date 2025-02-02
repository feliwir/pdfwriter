/*
 Source File : IResourceWritingTask.h


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
#pragma once

#include "EStatusCode.h"

class DictionaryContext;
class ObjectsContext;
namespace charta
{
class DocumentContext;
};

class IResourceWritingTask
{
  public:
    virtual ~IResourceWritingTask()
    {
    }

    virtual charta::EStatusCode Write(DictionaryContext *inResoruceCategoryContext, ObjectsContext *inObjectsContext,
                                      charta::DocumentContext *inDocumentContext) = 0;
};
