// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION( AttributeList )

AttributeList::AttributeList()
    : attributes_()
{
}

AttributeList::AttributeList(
    std::map<std::wstring, std::wstring> const& attributes)
    : attributes_(attributes)
{
}

AttributeList::AttributeList(AttributeList && other)
    : attributes_(move(other.attributes_))
{
}

AttributeList & AttributeList::operator = (AttributeList && other)
{
    if (this != &other)
    {
        attributes_ = move(other.attributes_);
    }

    return *this;
}

ErrorCode AttributeList::FromPublicApi(FABRIC_STRING_PAIR_LIST const & attributeList)
{
    if (attributeList.Count > 0)
    {
        for (ULONG i = 0; i < attributeList.Count; i++)
        {
            attributes_.insert(pair<wstring, wstring>(attributeList.Items[i].Name, attributeList.Items[i].Value));
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode AttributeList::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_STRING_PAIR_LIST & attributes) const
{
    ULONG count = static_cast<ULONG>(attributes_.size());
    auto nativeAttributes = heap.AddArray<FABRIC_STRING_PAIR>(count);
    size_t i = 0;
    for (auto it = attributes_.begin(); it != attributes_.end(); ++it, ++i)
    {
        nativeAttributes[i].Name = heap.AddString(it->first);
        nativeAttributes[i].Value = heap.AddString(it->second);
    }

    attributes.Count = count;
    attributes.Items = nativeAttributes.GetRawArray();

    return ErrorCode::Success();
}

void AttributeList::AddAttribute(
    std::wstring const & key,
    std::wstring const & value)
{
    attributes_.insert(pair<wstring, wstring>(key, value));
}

void AttributeList::WriteTo(TextWriter& writer, FormatOptions const &) const
{
    writer.Write("{0} attributes", attributes_.size());
}

std::wstring AttributeList::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

