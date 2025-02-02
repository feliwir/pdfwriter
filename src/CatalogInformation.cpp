/*
   Source File : CatalogInformation.cpp


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
#include "CatalogInformation.h"
#include "IndirectObjectsReferenceRegistry.h"
#include "PageTree.h"

CatalogInformation::CatalogInformation()
{
    mCurrentPageTreeNode = nullptr;
}

CatalogInformation::~CatalogInformation()
{
    Reset();
}

void CatalogInformation::Reset()
{
    if (mCurrentPageTreeNode != nullptr)
    {
        // delete root
        PageTree *resultPageTree = mCurrentPageTreeNode;
        while (resultPageTree->GetParent() != nullptr)
            resultPageTree = resultPageTree->GetParent();
        delete resultPageTree;
    }
    mCurrentPageTreeNode = nullptr;
}

ObjectIDType CatalogInformation::AddPageToPageTree(ObjectIDType inPageID,
                                                   IndirectObjectsReferenceRegistry &inObjectsRegistry)
{
    if (mCurrentPageTreeNode == nullptr)
        mCurrentPageTreeNode = new PageTree(inObjectsRegistry);

    mCurrentPageTreeNode = mCurrentPageTreeNode->AddNodeToTree(inPageID, inObjectsRegistry);
    return mCurrentPageTreeNode->GetID();
}

PageTree *CatalogInformation::GetPageTreeRoot(IndirectObjectsReferenceRegistry &inObjectsRegistry)
{
    if (mCurrentPageTreeNode != nullptr)
    {
        PageTree *resultPageTree = mCurrentPageTreeNode;
        while (resultPageTree->GetParent() != nullptr)
            resultPageTree = resultPageTree->GetParent();
        return resultPageTree;
    }

    mCurrentPageTreeNode = new PageTree(inObjectsRegistry);
    return mCurrentPageTreeNode;
}

PageTree *CatalogInformation::GetCurrentPageTreeNode()
{
    return mCurrentPageTreeNode;
}

void CatalogInformation::SetCurrentPageTreeNode(PageTree *inCurrentPageTreeNode)
{
    mCurrentPageTreeNode = inCurrentPageTreeNode;
}
