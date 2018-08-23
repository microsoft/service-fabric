// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationHealthPolicyMap::ApplicationHealthPolicyMap()
    : map_()
{
}

#if defined(PLATFORM_UNIX)
void ApplicationHealthPolicyMap::Insert(std::wstring & appName, ApplicationHealthPolicy && policy)
{
	map_.insert(make_pair<wstring&, ApplicationHealthPolicy&&>(appName, move(policy)));
}
#else
void ApplicationHealthPolicyMap::Insert(std::wstring const & appName, ApplicationHealthPolicy && policy)

{
    map_.insert(make_pair(appName, move(policy)));
}
#endif

std::shared_ptr<ApplicationHealthPolicy> ApplicationHealthPolicyMap::GetApplicationHealthPolicy(wstring const & applicationName) const
{
    auto iter  = map_.find(applicationName);
    if (iter == map_.end())
    {
        return nullptr;
    }
    else
    {
        return make_shared<ApplicationHealthPolicy>(iter->second);
    }
}

bool ApplicationHealthPolicyMap::TryValidate(__inout wstring & validationErrorMessage) const
{
    for (auto const & entry : map_)
    {
        if (!entry.second.TryValidate(validationErrorMessage))
        {
            return false;
        }
    }

    return true;
}

bool ApplicationHealthPolicyMap::operator == (ApplicationHealthPolicyMap const & other) const
{
    bool equals = (map_.size() == other.map_.size());
    if (!equals) return equals;

    for (auto const & entry : map_)
    {
        auto itOther = other.map_.find(entry.first);
        if (itOther == other.map_.end())
        {
            return false;
        }

        equals = (entry.second == itOther->second);
        if (!equals) return equals;
    }

    return equals;
}

bool ApplicationHealthPolicyMap::operator != (ApplicationHealthPolicyMap const & other) const
{
    return !(*this == other);
}

wstring ApplicationHealthPolicyMap::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ApplicationHealthPolicyMap&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ApplicationHealthPolicyMap::FromString(wstring const & applicationHealthPolicyStr, __out ApplicationHealthPolicyMap & ApplicationHealthPolicyMap)
{
    return JsonHelper::Deserialize(ApplicationHealthPolicyMap, applicationHealthPolicyStr);
}

ErrorCode ApplicationHealthPolicyMap::FromPublicApi(FABRIC_APPLICATION_HEALTH_POLICY_MAP const & publicAppHealthPolicyMap)
{
    return FromPublicApiMap(publicAppHealthPolicyMap, map_);
}

void ApplicationHealthPolicyMap::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_APPLICATION_HEALTH_POLICY_MAP & publicAppHealthPolicyMap) const
{
    publicAppHealthPolicyMap.Count = static_cast<ULONG>(map_.size());

    if (publicAppHealthPolicyMap.Count > 0)
    {
        auto mapItems = heap.AddArray<FABRIC_APPLICATION_HEALTH_POLICY_MAP_ITEM>(publicAppHealthPolicyMap.Count);
        publicAppHealthPolicyMap.Items = mapItems.GetRawArray();

        int index = 0;
        for (auto const & entry : map_)
        {
            entry.second.ToPublicApiMapItem(heap, entry.first, mapItems[index++]);
        }
    }
    else
    {
        publicAppHealthPolicyMap.Items = NULL;
    }
}

Common::ErrorCode ApplicationHealthPolicyMap::FromPublicApiMap(
    FABRIC_APPLICATION_HEALTH_POLICY_MAP const & publicAppHealthPolicyMap,
    __inout std::map<std::wstring, ApplicationHealthPolicy> & map)
{
    ErrorCode error(ErrorCodeValue::Success);
    for (size_t i = 0; i < publicAppHealthPolicyMap.Count; ++i)
    {
        auto publicApplicationHealthPolicyItem = publicAppHealthPolicyMap.Items[i];
        if (publicApplicationHealthPolicyItem.HealthPolicy == NULL)
        {
            return ErrorCode(ErrorCodeValue::ArgumentNull);
        }

        NamingUri appName;
        error = NamingUri::TryParse(
            publicApplicationHealthPolicyItem.ApplicationName,
            "ApplicationName",
            appName);
        if (!error.IsSuccess())
        {
            return error;
        }

        ApplicationHealthPolicy healthPolicy;
        error = healthPolicy.FromPublicApi(*publicApplicationHealthPolicyItem.HealthPolicy);
        if (!error.IsSuccess())
        {
            return error;
        }

        map.insert(make_pair<wstring, ApplicationHealthPolicy>(appName.ToString(), move(healthPolicy)));
    }

    return ErrorCode(ErrorCodeValue::Success);
}
