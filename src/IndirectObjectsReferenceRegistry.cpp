/*
   Source File : IndirectObjectsReferenceRegistry.cpp


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
#include "IndirectObjectsReferenceRegistry.h"
#include "DictionaryContext.h"
#include "ObjectsContext.h"
#include "Trace.h"
#include "objects/PDFArray.h"
#include "objects/PDFBoolean.h"
#include "objects/PDFDictionary.h"
#include "objects/PDFIndirectObjectReference.h"
#include "objects/PDFInteger.h"
#include "objects/PDFObjectCast.h"
#include "parsing/PDFParser.h"

#include <list>

using namespace charta;

IndirectObjectsReferenceRegistry::IndirectObjectsReferenceRegistry()
{
    SetupInitialFreeObject();
}

void IndirectObjectsReferenceRegistry::SetupInitialFreeObject()
{
    ObjectWriteInformation singleFreeObjectInformation;

    singleFreeObjectInformation.mObjectReferenceType = ObjectWriteInformation::Free;
    singleFreeObjectInformation.mObjectWritten = false;
    singleFreeObjectInformation.mIsDirty = true;
    singleFreeObjectInformation.mGenerationNumber = 65535;
    singleFreeObjectInformation.mWritePosition = 0;
    mObjectsWritesRegistry.push_back(singleFreeObjectInformation);
}

IndirectObjectsReferenceRegistry::~IndirectObjectsReferenceRegistry() = default;

ObjectIDType IndirectObjectsReferenceRegistry::AllocateNewObjectID()
{
    ObjectWriteInformation newObjectInformation;
    ObjectIDType newObjectID = GetObjectsCount();

    newObjectInformation.mObjectWritten = false;
    newObjectInformation.mObjectReferenceType = ObjectWriteInformation::Used;
    newObjectInformation.mGenerationNumber = 0;
    newObjectInformation.mIsDirty = true;

    mObjectsWritesRegistry.push_back(newObjectInformation);
    return newObjectID;
}

EStatusCode IndirectObjectsReferenceRegistry::MarkObjectAsWritten(ObjectIDType inObjectID, long long inWritePosition)
{
    if (mObjectsWritesRegistry.size() <= inObjectID)
    {
        TRACE_LOG1("IndirectObjectsReferenceRegistry::MarkObjectAsWritten, Out of range failure. An Object ID is "
                   "marked as written, which was not allocated before. ID = %ld",
                   inObjectID);
        return charta::eFailure;
    }

    if (mObjectsWritesRegistry[inObjectID].mObjectWritten)
    {
        TRACE_LOG3("IndirectObjectsReferenceRegistry::MarkObjectAsWritten, Object rewrite failure. The object %ld was "
                   "already marked as written at %lld. New position is %lld",
                   inObjectID, mObjectsWritesRegistry[inObjectID].mWritePosition, inWritePosition);
        return charta::eFailure; // trying to mark as written an object that was already marked as such in the past.
                                 // probably a mistake [till we have revisions]
    }

    if (inWritePosition >
        9999999999LL) // if write position is larger than what can be represented by 10 digits, xref write will fail
    {
        TRACE_LOG1(
            "IndirectObjectsReferenceRegistry::MarkObjectAsWritten, Write position out of bounds. Trying to write an "
            "object at position that cannot be represented in Xref = %lld. probably means file got too long",
            inWritePosition);
        return charta::eFailure;
    }

    mObjectsWritesRegistry[inObjectID].mIsDirty = true;
    mObjectsWritesRegistry[inObjectID].mWritePosition = inWritePosition;
    mObjectsWritesRegistry[inObjectID].mObjectWritten = true;
    return charta::eSuccess;
}

GetObjectWriteInformationResult IndirectObjectsReferenceRegistry::GetObjectWriteInformation(
    ObjectIDType inObjectID) const
{
    GetObjectWriteInformationResult result;

    if (mObjectsWritesRegistry.size() <= inObjectID)
    {
        result.first = false;
    }
    else
    {
        result.first = true;
        result.second = mObjectsWritesRegistry[inObjectID];
    }
    return result;
}

const ObjectWriteInformation &IndirectObjectsReferenceRegistry::GetNthObjectReference(ObjectIDType inObjectID) const
{
    return mObjectsWritesRegistry[inObjectID];
}

ObjectIDType IndirectObjectsReferenceRegistry::GetObjectsCount() const
{
    return static_cast<ObjectIDType>(mObjectsWritesRegistry.size());
}

charta::EStatusCode IndirectObjectsReferenceRegistry::DeleteObject(ObjectIDType inObjectID)
{
    if (mObjectsWritesRegistry.size() <= inObjectID)
    {
        TRACE_LOG1("IndirectObjectsReferenceRegistry::DeleteObject, Out of range failure. An Object ID is marked for "
                   "delete,but there's no such object. ID = %ld",
                   inObjectID);
        return charta::eFailure;
    }

    if (mObjectsWritesRegistry[inObjectID].mGenerationNumber == 65535)
    {
        TRACE_LOG1("IndirectObjectsReferenceRegistry::DeleteObject, object ID generation number reached maximum value "
                   "and cannot be increased. ID = %ld",
                   inObjectID);
        return charta::eFailure;
    }

    mObjectsWritesRegistry[inObjectID].mIsDirty = true;
    ++(mObjectsWritesRegistry[inObjectID].mGenerationNumber);
    mObjectsWritesRegistry[inObjectID].mWritePosition = 0;
    mObjectsWritesRegistry[inObjectID].mObjectReferenceType = ObjectWriteInformation::Free;

    return charta::eSuccess;
}

charta::EStatusCode IndirectObjectsReferenceRegistry::MarkObjectAsUpdated(ObjectIDType inObjectID,
                                                                          long long inNewWritePosition)
{
    if (mObjectsWritesRegistry.size() <= inObjectID)
    {
        TRACE_LOG1("IndirectObjectsReferenceRegistry::MarkObjectAsUpdated, Out of range failure. An Object ID is "
                   "marked for update,but there's no such object. ID = %ld",
                   inObjectID);
        return charta::eFailure;
    }

    if (inNewWritePosition >
        9999999999LL) // if write position is larger than what can be represented by 10 digits, xref write will fail
    {
        TRACE_LOG1(
            "IndirectObjectsReferenceRegistry::MarkObjectAsUpdated, Write position out of bounds. Trying to write an "
            "object at position that cannot be represented in Xref = %lld. probably means file got too long",
            inNewWritePosition);
        return charta::eFailure;
    }

    mObjectsWritesRegistry[inObjectID].mIsDirty = true;
    mObjectsWritesRegistry[inObjectID].mWritePosition = inNewWritePosition;
    mObjectsWritesRegistry[inObjectID].mObjectReferenceType = ObjectWriteInformation::Used;

    return charta::eSuccess;
}

using ObjectIDTypeList = std::list<ObjectIDType>;

EStatusCode IndirectObjectsReferenceRegistry::WriteState(ObjectsContext *inStateWriter, ObjectIDType inObjectID)
{
    ObjectIDTypeList objects;

    inStateWriter->StartNewIndirectObject(inObjectID);

    DictionaryContext *myDictionary = inStateWriter->StartDictionary();

    myDictionary->WriteKey("Type");
    myDictionary->WriteNameValue("IndirectObjectsReferenceRegistry");

    myDictionary->WriteKey("mObjectsWritesRegistry");

    auto it = mObjectsWritesRegistry.begin();

    inStateWriter->StartArray();
    for (; it != mObjectsWritesRegistry.end(); ++it)
    {
        ObjectIDType objectWriteEntry = inStateWriter->GetInDirectObjectsRegistry().AllocateNewObjectID();
        inStateWriter->WriteIndirectObjectReference(objectWriteEntry);
        objects.push_back(objectWriteEntry);
    }
    inStateWriter->EndArray(eTokenSeparatorEndLine);

    inStateWriter->EndDictionary(myDictionary);
    inStateWriter->EndIndirectObject();

    auto itIDs = objects.begin();

    it = mObjectsWritesRegistry.begin();

    for (; it != mObjectsWritesRegistry.end(); ++it, ++itIDs)
    {
        inStateWriter->StartNewIndirectObject(*itIDs);

        DictionaryContext *registryDictionary = inStateWriter->StartDictionary();

        registryDictionary->WriteKey("Type");
        registryDictionary->WriteNameValue("ObjectWriteInformation");

        registryDictionary->WriteKey("mObjectWritten");
        registryDictionary->WriteBooleanValue(it->mObjectWritten);

        if (it->mObjectWritten)
        {
            registryDictionary->WriteKey("mWritePosition");
            registryDictionary->WriteIntegerValue(it->mWritePosition);
        }

        registryDictionary->WriteKey("mObjectReferenceType");
        registryDictionary->WriteIntegerValue(it->mObjectReferenceType);

        registryDictionary->WriteKey("mIsDirty");
        registryDictionary->WriteBooleanValue(it->mIsDirty);

        registryDictionary->WriteKey("mGenerationNumber");
        registryDictionary->WriteIntegerValue(it->mGenerationNumber);

        inStateWriter->EndDictionary(registryDictionary);
        inStateWriter->EndIndirectObject();
    }

    return charta::eSuccess;
}

EStatusCode IndirectObjectsReferenceRegistry::ReadState(PDFParser *inStateReader, ObjectIDType inObjectID)
{
    PDFObjectCastPtr<charta::PDFDictionary> indirectObjectsDictionary(inStateReader->ParseNewObject(inObjectID));

    PDFObjectCastPtr<charta::PDFArray> objectsWritesRegistry(
        indirectObjectsDictionary->QueryDirectObject("mObjectsWritesRegistry"));

    auto it = objectsWritesRegistry->GetIterator();

    mObjectsWritesRegistry.clear();
    while (it.MoveNext())
    {
        ObjectWriteInformation newObjectInformation;
        PDFObjectCastPtr<charta::PDFDictionary> objectWriteInformationDictionary(inStateReader->ParseNewObject(
            std::static_pointer_cast<charta::PDFIndirectObjectReference>(it.GetItem())->mObjectID));

        PDFObjectCastPtr<charta::PDFBoolean> objectWritten(
            objectWriteInformationDictionary->QueryDirectObject("mObjectWritten"));

        newObjectInformation.mObjectWritten = objectWritten->GetValue();

        if (newObjectInformation.mObjectWritten)
        {
            PDFObjectCastPtr<PDFInteger> writePosition(
                objectWriteInformationDictionary->QueryDirectObject("mWritePosition"));
            newObjectInformation.mWritePosition = writePosition->GetValue();
        }

        PDFObjectCastPtr<PDFInteger> objectReferenceType(
            objectWriteInformationDictionary->QueryDirectObject("mObjectReferenceType"));
        newObjectInformation.mObjectReferenceType =
            (ObjectWriteInformation::EObjectReferenceType)objectReferenceType->GetValue();

        PDFObjectCastPtr<charta::PDFBoolean> objectDirty(
            objectWriteInformationDictionary->QueryDirectObject("mIsDirty"));
        newObjectInformation.mIsDirty = objectDirty->GetValue();

        PDFObjectCastPtr<PDFInteger> generationNumber(
            objectWriteInformationDictionary->QueryDirectObject("mGenerationNumber"));
        newObjectInformation.mGenerationNumber = (unsigned long)generationNumber->GetValue();

        mObjectsWritesRegistry.push_back(newObjectInformation);
    }

    return charta::eSuccess;
}

void IndirectObjectsReferenceRegistry::Reset()
{
    mObjectsWritesRegistry.clear();

    SetupInitialFreeObject();
}

void IndirectObjectsReferenceRegistry::AppendExistingItem(
    ObjectWriteInformation::EObjectReferenceType inObjectReferenceType, unsigned long inGenerationNumber,
    long long inWritePosition)
{

    ObjectWriteInformation newObjectInformation;

    newObjectInformation.mObjectWritten = (inObjectReferenceType == ObjectWriteInformation::Used);
    newObjectInformation.mObjectReferenceType = inObjectReferenceType;
    newObjectInformation.mGenerationNumber = inGenerationNumber;
    newObjectInformation.mIsDirty = false;
    newObjectInformation.mWritePosition = (inObjectReferenceType == ObjectWriteInformation::Used) ? inWritePosition : 0;

    mObjectsWritesRegistry.push_back(newObjectInformation);
}

void IndirectObjectsReferenceRegistry::SetupXrefFromModifiedFile(PDFParser *inModifiedFileParser)
{

    // kind of easy, just read the xref from the parer into the existing parser [skip first element, which is the free
    // element]
    for (ObjectIDType i = 1; i < inModifiedFileParser->GetXrefSize(); ++i)
    {
        XrefEntryInput *anEntry = inModifiedFileParser->GetXrefEntry(i);
        AppendExistingItem(anEntry->mType != eXrefEntryDelete ? ObjectWriteInformation::Used
                                                              : ObjectWriteInformation::Free,
                           anEntry->mType != eXrefEntryStreamObject ? anEntry->mRivision : 0, anEntry->mObjectPosition);
    }
}
