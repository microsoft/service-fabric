// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::HealthManager;

StringLiteral const TraceSource("ClusterUpgradeStateSnapshot");

UnhealthyState::UnhealthyState()
    : ErrorCount(0)
    , TotalCount(0)
{
}

UnhealthyState::UnhealthyState(ULONG errorCount, ULONG totalCount)
    : ErrorCount(errorCount)
    , TotalCount(totalCount)
{
}

bool UnhealthyState::operator == (UnhealthyState const & other) const
{
    return ErrorCount == other.ErrorCount && TotalCount == other.TotalCount;
}

bool UnhealthyState::operator != (UnhealthyState const & other) const
{
    return !(*this == other);
}

float UnhealthyState::GetUnhealthyPercent() const
{
    return GetUnhealthyPercent(ErrorCount, TotalCount);
}

float UnhealthyState::GetUnhealthyPercent(ULONG errorCount, ULONG totalCount)
{
    if (totalCount == 0) 
    {
        return 0.0f;
    }

    return static_cast<float>(errorCount)* 100.0f / static_cast<float>(totalCount);
}

ClusterUpgradeStateSnapshot::ClusterUpgradeStateSnapshot()
    : globalUnhealthyState_()
    , upgradeDomainUnhealthyStates_()
{
}

ClusterUpgradeStateSnapshot::~ClusterUpgradeStateSnapshot()
{
}

ClusterUpgradeStateSnapshot::ClusterUpgradeStateSnapshot(ClusterUpgradeStateSnapshot && other)
    : globalUnhealthyState_(move(other.globalUnhealthyState_))
    , upgradeDomainUnhealthyStates_(move(other.upgradeDomainUnhealthyStates_))
{
}
            
ClusterUpgradeStateSnapshot & ClusterUpgradeStateSnapshot::operator = (ClusterUpgradeStateSnapshot && other)
{
    if (this != &other)
    {
        globalUnhealthyState_ = move(other.globalUnhealthyState_);
        upgradeDomainUnhealthyStates_ = move(other.upgradeDomainUnhealthyStates_);
    }

    return *this;
}

void ClusterUpgradeStateSnapshot::SetGlobalState(ULONG errorCount, ULONG totalCount)
{
    globalUnhealthyState_.ErrorCount = errorCount;
    globalUnhealthyState_.TotalCount = totalCount;
}

bool ClusterUpgradeStateSnapshot::IsValid() const
{
    return globalUnhealthyState_.TotalCount > 0 && upgradeDomainUnhealthyStates_.size() > 0;
}

void ClusterUpgradeStateSnapshot::AddUpgradeDomainEntry(
    std::wstring const & upgradeDomainName,
    ULONG errorCount,
    ULONG totalCount)
{
    auto result = upgradeDomainUnhealthyStates_.insert(
        UpgradeDomainStatusPair(upgradeDomainName, UnhealthyState(errorCount, totalCount)));
    if (!result.second)
    {
        // The item is already in the map
        result.first->second.ErrorCount = errorCount;
        result.first->second.TotalCount = totalCount;
    }
}

bool ClusterUpgradeStateSnapshot::HasUpgradeDomainEntry(std::wstring const & upgradeDomainName) const
{
    return upgradeDomainUnhealthyStates_.find(upgradeDomainName) != upgradeDomainUnhealthyStates_.end();
}

Common::ErrorCode ClusterUpgradeStateSnapshot::TryGetUpgradeDomainEntry(
    std::wstring const & upgradeDomainName,
    __inout ULONG & errorCount,
    __inout ULONG & totalCount) const
{
    auto it = upgradeDomainUnhealthyStates_.find(upgradeDomainName);
    if (it == upgradeDomainUnhealthyStates_.end())
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    errorCount = it->second.ErrorCount;
    totalCount = it->second.TotalCount;
    return ErrorCode::Success();
}

bool ClusterUpgradeStateSnapshot::IsGlobalDeltaRespected(
    ULONG errorCount,
    ULONG totalCount,
    BYTE maxPercentDelta) const
{
    return IsDeltaRespected(
        GlobalUnhealthyState.ErrorCount, 
        GlobalUnhealthyState.TotalCount, 
        errorCount, 
        totalCount, 
        maxPercentDelta);
}

bool ClusterUpgradeStateSnapshot::IsDeltaRespected(
    ULONG baselineErrorCount,
    ULONG baselineTotalCount,
    ULONG errorCount,
    ULONG totalCount,
    BYTE maxPercentDelta) 
{
    float baseUnhealthyPercent = UnhealthyState::GetUnhealthyPercent(baselineErrorCount, baselineTotalCount);
    float newUnhealthyPercent = UnhealthyState::GetUnhealthyPercent(errorCount, totalCount);
    return newUnhealthyPercent - baseUnhealthyPercent <= static_cast<float>(maxPercentDelta);
}

void ClusterUpgradeStateSnapshot::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    if (!IsValid())
    {
        w.Write("-Invalid-");
    }
    else
    {
        w.Write("global:{0}/{1}, UDs:", globalUnhealthyState_.ErrorCount, globalUnhealthyState_.TotalCount);
        for (auto const & entry : upgradeDomainUnhealthyStates_)
        {
            w.Write(" {0}:{1}/{2}", entry.first, entry.second.ErrorCount, entry.second.TotalCount);
        }
    }    
}
