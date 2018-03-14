// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ::Query;
using namespace ServiceModel;

QueryArgumentMap::QueryArgumentMap()
    : queryArgumentMap_()
{}

void QueryArgumentMap::Insert(wstring const & key, wstring const & value)
{
    queryArgumentMap_.insert(make_pair(QueryArgumentMapKey(key), value));
}

std::wstring QueryArgumentMap::operator[](std::wstring const & key) const
{
    wstring value;
    TryGetValue(key, value);
    return value;
}

bool QueryArgumentMap::ContainsKey(wstring const & key) const
{
    return queryArgumentMap_.find(QueryArgumentMapKey(key)) != queryArgumentMap_.end();
}

void QueryArgumentMap::Erase(wstring const & key)
{
    queryArgumentMap_.erase(QueryArgumentMapKey(key));
}

std::wstring QueryArgumentMap::FirstKey()
{
    return queryArgumentMap_.begin()->first.Key;
}

bool QueryArgumentMap::TryGetValue(wstring const & key, __out wstring & value) const
{
    auto entry = queryArgumentMap_.find(QueryArgumentMapKey(key));
    if (entry == queryArgumentMap_.end()) { return false; }
    value = entry->second;
    return true;
}


bool QueryArgumentMap::TryGetInt64(wstring const & key, __out int64 & value) const
{
    wstring foundValueString;

    if (TryGetValue(key, foundValueString))
    {
        // Convert to int64
        auto int64Parser = StringUtility::ParseIntegralString<int64, true>();

        // Return true if it was able to be parsed.
        return int64Parser.Try(foundValueString, value, 0);
    }

    return false;
}

bool QueryArgumentMap::TryGetValue(wstring const & key, __out int64 & value) const
{
    wstring stringVal;
    if (TryGetValue(key, stringVal))
    {
        if (Common::TryParseInt64(stringVal, value))
        {
            return true;
        }
    }

    return false;
}

Common::ErrorCode QueryArgumentMap::GetInt64(wstring const & key, __out int64 & value) const
{
    wstring foundValueString;
    bool valueIsFound = TryGetValue(key, foundValueString);

    if (!valueIsFound)
    {
        return ErrorCodeValue::NotFound;
    }

    // Convert to int64
    auto int64Parser = StringUtility::ParseIntegralString<int64, true>();

    if (int64Parser.Try(foundValueString, value, 0))
    {
        return ErrorCodeValue::Success;
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }
}

Common::ErrorCode QueryArgumentMap::GetBool(wstring const & key, __out bool & value) const
{
    wstring foundValueString;
    bool valueIsFound = TryGetValue(key, foundValueString);

    if (!valueIsFound)
    {
        return ErrorCodeValue::NotFound;
    }

    // TryFromWString returns true if it was able to be parsed.
    if (StringUtility::TryFromWString<bool>(foundValueString, value))
    {
        return ErrorCodeValue::Success;
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }
}

bool QueryArgumentMap::TryGetBool(wstring const & key, __out bool & value) const
{
    wstring foundValueString;
    bool valueIsFound = TryGetValue(key, foundValueString);

    if (valueIsFound)
    {
        // Return true if it was able to be parsed.
        return StringUtility::TryFromWString<bool>(foundValueString, value);
    }

    return false;
}

bool QueryArgumentMap::TryGetValue(wstring const & key, __out Guid & value) const
{
    wstring stringVal;
    if (!TryGetValue(key, stringVal))
    {
        return false;
    }

    if (stringVal.empty())
    {
        return false;
    }

    if (!Guid::TryParse(stringVal, value))
    {
        return false;
    }

    return true;
}

bool QueryArgumentMap::TryGetPartitionId(__out Guid & value) const
{
    return TryGetValue(QueryResourceProperties::Partition::PartitionId, value);
}

bool QueryArgumentMap::TryGetReplicaId(__out int64 & value) const
{
    return TryGetValue(QueryResourceProperties::Replica::ReplicaId, value);
}

void QueryArgumentMap::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_STRING_PAIR_LIST & publicNodeQueryArgs) const
{
    publicNodeQueryArgs.Count = 0;

    if (queryArgumentMap_.size() > 0)
    {
        auto argsArray = heap.AddArray<FABRIC_STRING_PAIR>(queryArgumentMap_.size());

        size_t i = 0;
        for (auto itr = queryArgumentMap_.begin(); itr != queryArgumentMap_.end(); itr++)
        {
            argsArray[i].Name = heap.AddString(itr->first.Key);
            argsArray[i].Value = heap.AddString(itr->second);
            i++;
        }

        publicNodeQueryArgs.Count = static_cast<ULONG>(queryArgumentMap_.size());
        publicNodeQueryArgs.Items = argsArray.GetRawArray();
    }
}

void QueryArgumentMap::WriteTo(TextWriter & w, FormatOptions const &) const
{
    for (auto itr = queryArgumentMap_.begin(); itr != queryArgumentMap_.end(); ++itr)
    {
        w.Write("{0}:{1} ", itr->first.Key, itr->second);
    }
}

string QueryArgumentMap::AddField(Common::TraceEvent &traceEvent, string const& name)
{
    traceEvent.AddField<wstring>(name);
    return string();
}

void QueryArgumentMap::FillEventData(TraceEventContext &context) const
{
    wstring string;
    StringWriter writer(string);

    for (auto itr = queryArgumentMap_.begin(); itr != queryArgumentMap_.end(); ++itr)
    {
        writer.Write("{0}:{1} ", itr->first.Key, itr->second);
    }

    context.WriteCopy(string);
}
