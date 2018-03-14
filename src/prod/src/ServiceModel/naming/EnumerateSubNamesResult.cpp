// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    DEFINE_SERIALIZATION_OVERHEAD_ESTIMATE( EnumerateSubNamesResult )

    EnumerateSubNamesResult::EnumerateSubNamesResult()
        : name_()
        , enumerationStatus_()
        , subNames_()
        , continuationToken_()
    {
    }

    EnumerateSubNamesResult::EnumerateSubNamesResult(
        NamingUri const & name,
        ::FABRIC_ENUMERATION_STATUS enumerationStatus,
        vector<wstring> && subNames,
        EnumerateSubNamesToken const & continuationToken)
        : name_(name)
        , enumerationStatus_(enumerationStatus)
        , subNames_(move(subNames))
        , continuationToken_(continuationToken)
    {
    }

    EnumerateSubNamesResult::EnumerateSubNamesResult(
        EnumerateSubNamesResult const & other)
        : name_(other.name_)
        , enumerationStatus_(other.enumerationStatus_)
        , subNames_(other.subNames_)
        , continuationToken_(other.continuationToken_)
    {
    }

    size_t EnumerateSubNamesResult::EstimateSerializedSize(NamingUri const & parentName, NamingUri const & subName)
    {
        return
            parentName.EstimateDynamicSerializationSize() +
            subName.EstimateDynamicSerializationSize() +
            (subName.ToString().size() * sizeof(wchar_t)) + // subname vector entry
            SerializationOverheadEstimate;
    }

    size_t EnumerateSubNamesResult::EstimateSerializedSize(size_t uriLength)
    {
        return
            (uriLength * sizeof(wchar_t) * 3) + // parentName URI, subName continuation token, subname vector entry
            SerializationOverheadEstimate;
    }
}
