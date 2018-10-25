// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <cstddef>

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("EventContextMap");

EventContextMap::EventContextMap()
: map_()
{
}

ErrorCode EventContextMap::FromPublicApi(FABRIC_EVENT_CONTEXT_MAP const & publicEventContextMap)
{
    return FromPublicApiMap(publicEventContextMap, map_);
}

Common::ErrorCode EventContextMap::FromPublicApiMap(
    FABRIC_EVENT_CONTEXT_MAP const & publicEventContextMap,
    __inout std::map<wstring, wstring> & map)
{
    ErrorCode error(ErrorCodeValue::Success);

    for (size_t i = 0; i < publicEventContextMap.Count; ++i)
    {
        auto publicEventContextMapItem = publicEventContextMap.Items[i];

        if (publicEventContextMapItem.Key == NULL || publicEventContextMapItem.Value == NULL)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().ChaosContextNullKeyOrValue));
        }

        // Key
        wstring key;
        error = StringUtility::LpcwstrToWstring2(
            publicEventContextMapItem.Key,
            true,
            0,
            ParameterValidator::MaxStringSize,
            key);
        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceComponent, "EventContextMap::FromPublicApi/Failed to parse key: {0}", error);
            return error;
        }

        // Value
        wstring value;
        error = StringUtility::LpcwstrToWstring2(
            publicEventContextMapItem.Value,
            true,
            0,
            ParameterValidator::MaxStringSize,
            value);
        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceComponent, "EventContextMap::FromPublicApi/Failed to parse value: {0}", error);
            return error;
        }

        map.insert(make_pair(key, value));
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode EventContextMap::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_EVENT_CONTEXT_MAP & result) const
{
    result.Count = 0;

    if (map_.size() > 0)
    {
        auto itemsArray = heap.AddArray<FABRIC_EVENT_CONTEXT_MAP_ITEM>(map_.size());

        size_t i = 0;
        for (auto itr = map_.begin(); itr != map_.end(); itr++)
        {
            itemsArray[i].Key = heap.AddString(itr->first);
            itemsArray[i].Value = heap.AddString(itr->second);
            i++;
        }

        result.Count = static_cast<ULONG>(map_.size());
        result.Items = itemsArray.GetRawArray();
    }

    return ErrorCode(ErrorCodeValue::Success);
}
