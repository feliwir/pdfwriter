/*
   Source File : PDFObjectCast.h


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

#include "PDFObject.h"

/*
    This small template function is only intended to be used for automatic casting of retrieved PDFObjects to their
   respective actual objects...and not for anything else.
*/

template <class T> std::shared_ptr<T> PDFObjectCast(std::shared_ptr<charta::PDFObject> inOriginal)
{
    using namespace charta;
    if (!inOriginal)
        return nullptr;

    if (inOriginal->GetType() == (charta::PDFObject::EPDFObjectType)T::eType)
    {
        return std::static_pointer_cast<T>(inOriginal);
    }
    else
    {
        return nullptr;
    }
}

template <class T> class PDFObjectCastPtr
{
  private:
    std::shared_ptr<T> mValue;

  public:
    PDFObjectCastPtr()
    {
    }

    PDFObjectCastPtr(std::shared_ptr<charta::PDFObject> inPDFObject)
    {
        mValue = PDFObjectCast<T>(inPDFObject);
    }

    PDFObjectCastPtr<T> &operator=(std::shared_ptr<charta::PDFObject> inValue)
    {
        mValue = PDFObjectCast<T>(inValue);
        return *this;
    }

    T *operator->()
    {
        return mValue.get();
    }

    T *GetPtr() const
    {
        return mValue.get();
    }

    bool operator!() const
    {
        return mValue == nullptr;
    }

    // implicit conversion to shared_ptr
    operator std::shared_ptr<T>() const
    {
        return mValue;
    }

    bool operator!=(const std::nullptr_t &b) const
    {
        return mValue != b;
    }
};
