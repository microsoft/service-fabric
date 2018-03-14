// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;


void StringList::ToPublicAPI(
    __in Common::ScopedHeap & heap, 
    __in Common::StringCollection const& stringCollection, 
    __out Common::ReferencePointer<FABRIC_STRING_LIST> & referencePointer)
{
    auto count = stringCollection.size();
    referencePointer->Count = static_cast<ULONG>(count);

    if (count > 0)
    {
        auto items = heap.AddArray<LPCWSTR>(count);

        for (size_t i = 0; i < items.GetCount(); ++i)
        {
            items[i] = heap.AddString(stringCollection[i]);
        }

        referencePointer->Items = items.GetRawArray();
    }
    else
    {
        referencePointer->Items = NULL;
    }
}

Common::ErrorCode StringList::FromPublicApi(__in FABRIC_STRING_LIST const &publicStringList, __out Common::StringCollection & stringCollection)
{
    for (ULONG i = 0; i < publicStringList.Count; ++i)
    {
        stringCollection.push_back(wstring(publicStringList.Items[i]));
    }

    return ErrorCode::Success();
}

void StringList::WriteTo(
    __in Common::TextWriter & w, 
    std::wstring const & stringCollectionDescription,
    Common::StringCollection const & v)
{
    w << stringCollectionDescription << " = { ";
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        w << *it << " ";
    }
    w << "} ";
}

std::wstring StringList::ToString(
    std::wstring const & stringCollectionDescription,
    Common::StringCollection const & v)
{
    wstring collectionContent = wformatString("{0} = { ", stringCollectionDescription);
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        collectionContent.append(*it);
        collectionContent.append(L" ");
    }
    collectionContent.append(L"} ");
    return collectionContent;
}
