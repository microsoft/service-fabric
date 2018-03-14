// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    PagedPropertyInfoList::PagedPropertyInfoList()
        : continuationToken_()
        , isConsistent_(false)
        , properties_()
    {
    }

    PagedPropertyInfoList::PagedPropertyInfoList(
        EnumeratePropertiesResult && enumeratePropertiesResult,
        std::wstring && continuationToken,
        bool includeValues)
        : continuationToken_(std::move(continuationToken))
        , isConsistent_(false)
        , properties_()
    {
        if ((enumeratePropertiesResult.FABRIC_ENUMERATION_STATUS & FABRIC_ENUMERATION_CONSISTENT_MASK) != 0)
        {
            isConsistent_ = true;
        }

        std::vector<NamePropertyResult> properties = enumeratePropertiesResult.TakeProperties();
        for (auto && value : properties)
        {
            properties_.push_back(PropertyInfo::Create(std::move(value), includeValues));
        }
    }
}
