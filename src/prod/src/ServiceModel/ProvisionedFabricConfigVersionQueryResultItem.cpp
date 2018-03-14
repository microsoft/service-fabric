// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ProvisionedFabricConfigVersionQueryResultItem::ProvisionedFabricConfigVersionQueryResultItem()
    : configVersion_()
{
}

ProvisionedFabricConfigVersionQueryResultItem::ProvisionedFabricConfigVersionQueryResultItem(wstring const & configVersion)
    : configVersion_(configVersion)
{
}

ProvisionedFabricConfigVersionQueryResultItem::ProvisionedFabricConfigVersionQueryResultItem(ProvisionedFabricConfigVersionQueryResultItem const & other)
    : configVersion_(other.configVersion_)
{
}

ProvisionedFabricConfigVersionQueryResultItem::ProvisionedFabricConfigVersionQueryResultItem(ProvisionedFabricConfigVersionQueryResultItem && other)
    : configVersion_(move(other.configVersion_))
{
}

ProvisionedFabricConfigVersionQueryResultItem const & ProvisionedFabricConfigVersionQueryResultItem::operator = (ProvisionedFabricConfigVersionQueryResultItem const & other)
{
    if (this != &other)
    {
        this->configVersion_ = other.configVersion_;
    }

    return *this;
}

ProvisionedFabricConfigVersionQueryResultItem const & ProvisionedFabricConfigVersionQueryResultItem::operator = (ProvisionedFabricConfigVersionQueryResultItem && other)
{
    if (this != &other)
    {
        this->configVersion_ = move(other.configVersion_);
    }

    return *this;
}

void ProvisionedFabricConfigVersionQueryResultItem::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_ITEM & publicQueryResult) const 
{
    publicQueryResult.ConfigVersion = heap.AddString(configVersion_);
}

void ProvisionedFabricConfigVersionQueryResultItem::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ProvisionedFabricConfigVersionQueryResultItem::ToString() const
{
    return wformatString("ConfigVersion = '{0}'", configVersion_); 
}
