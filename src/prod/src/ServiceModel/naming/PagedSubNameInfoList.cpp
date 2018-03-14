// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    PagedSubNameInfoList::PagedSubNameInfoList()
        : EnumerateSubNamesResult()
        , continuationTokenString_()
        , isConsistent_(false)
    {
    }

    PagedSubNameInfoList::PagedSubNameInfoList(
        EnumerateSubNamesResult && enumerateSubNamesResult,
        std::wstring && continuationTokenString)
        : EnumerateSubNamesResult(std::move(enumerateSubNamesResult))
        , continuationTokenString_(std::move(continuationTokenString))
    {
        isConsistent_ = false;
        if ((FABRIC_ENUMERATION_STATUS & FABRIC_ENUMERATION_CONSISTENT_MASK) != 0)
        {
            isConsistent_ = true;
        }
    }
}
