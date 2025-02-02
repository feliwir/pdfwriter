/*
   Source File : PDFDictionary.cpp


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
#include "objects/PDFDictionary.h"

charta::PDFDictionary::PDFDictionary() : PDFObject(eType)
{
}

std::shared_ptr<charta::PDFObject> charta::PDFDictionary::QueryDirectObject(const std::string &inName)
{
    auto key = std::make_shared<PDFName>(inName);
    auto it = mValues.find(key);

    if (it == mValues.end())
    {
        return nullptr;
    }

    return it->second;
}

void charta::PDFDictionary::Insert(const std::shared_ptr<charta::PDFName> &inKeyObject,
                                   const std::shared_ptr<charta::PDFObject> &inValueObject)
{
    mValues.insert({inKeyObject, inValueObject});
}

bool charta::PDFDictionary::Exists(const std::string &inName)
{
    auto key = std::make_shared<PDFName>(inName);
    return mValues.find(key) != mValues.end();
}

MapIterator<std::map<std::shared_ptr<charta::PDFName>, std::shared_ptr<charta::PDFObject>, charta::PDFNameLess>>
charta::PDFDictionary::GetIterator()
{
    return {mValues};
}