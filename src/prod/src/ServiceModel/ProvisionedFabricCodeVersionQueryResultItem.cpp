// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ProvisionedFabricCodeVersionQueryResultItem::ProvisionedFabricCodeVersionQueryResultItem()
    : codeVersion_()
{
}

ProvisionedFabricCodeVersionQueryResultItem::ProvisionedFabricCodeVersionQueryResultItem(wstring const & codeVersion)
    : codeVersion_(codeVersion)
{
}

ProvisionedFabricCodeVersionQueryResultItem::ProvisionedFabricCodeVersionQueryResultItem(ProvisionedFabricCodeVersionQueryResultItem const & other)
    : codeVersion_(other.codeVersion_)
{
}

ProvisionedFabricCodeVersionQueryResultItem::ProvisionedFabricCodeVersionQueryResultItem(ProvisionedFabricCodeVersionQueryResultItem && other)
    : codeVersion_(move(other.codeVersion_))
{
}

ProvisionedFabricCodeVersionQueryResultItem const & ProvisionedFabricCodeVersionQueryResultItem::operator = (ProvisionedFabricCodeVersionQueryResultItem const & other)
{
    if (this != &other)
    {
        this->codeVersion_ = other.codeVersion_;
    }

    return *this;
}

ProvisionedFabricCodeVersionQueryResultItem const & ProvisionedFabricCodeVersionQueryResultItem::operator = (ProvisionedFabricCodeVersionQueryResultItem && other)
{
    if (this != &other)
    {
        this->codeVersion_ = move(other.codeVersion_);
    }

    return *this;
}

void ProvisionedFabricCodeVersionQueryResultItem::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_ITEM & publicQueryResult) const 
{
    publicQueryResult.CodeVersion = heap.AddString(codeVersion_);
}

void ProvisionedFabricCodeVersionQueryResultItem::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ProvisionedFabricCodeVersionQueryResultItem::ToString() const
{
    return wformatString("CodeVersion = '{0}'", codeVersion_); 
}
