// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

TimeSpan UpgradeHelper::GetReplicaSetCheckTimeoutForInitialUpgrade(DWORD wrapperTimeout)
{
    // Public API struct uses DWORD to express timeout in seconds, which is unsigned.
    //
    // Default structs in native will be initialized to 0, which should default to a max-value
    // safety check timeout (the default value for this undefined JSON member is also 0).
    //
    // Managed code will explicitly set the timeout to int.MaxValue, which should also default 
    // to a max-value safety check timeout.
    //
    return (wrapperTimeout > 0 && wrapperTimeout < numeric_limits<DWORD>::max() 
        ? TimeSpan::FromSeconds(wrapperTimeout)
        : TimeSpan::MaxValue);
}

TimeSpan UpgradeHelper::GetReplicaSetCheckTimeoutForModify(TimeSpan const timeout)
{
    // Allow setting the replica set check timeout to 0 during modification.
    //
    return (timeout < TimeSpan::FromSeconds(numeric_limits<DWORD>::max())
        ? timeout
        : TimeSpan::MaxValue);
}

DWORD UpgradeHelper::ToPublicReplicaSetCheckTimeoutInSeconds(TimeSpan const timeout)
{
    if (timeout == TimeSpan::MaxValue)
    {
        return numeric_limits<DWORD>::max();
    }
    else
    {
        return static_cast<DWORD>(timeout.TotalSeconds());
    }
}

void UpgradeHelper::GetChangedUpgradeDomains(
    wstring const& currentInProgress,
    vector<wstring> const& currentCompleted,
    wstring const& prevInProgress,
    vector<wstring> const& prevCompleted,
    __out wstring &changedInProgressDomain,
    __out vector<wstring> &changedCompletedDomains)
{
    changedCompletedDomains.clear();
    changedInProgressDomain.clear();

    //
    // Changes to Completed - domains that are currently completed but 
    // were not in the completed state in the prev progress.
    // 
    for (auto currentCompletedDomainItr = currentCompleted.begin();
         currentCompletedDomainItr != currentCompleted.end();
         ++currentCompletedDomainItr)
    {
        bool changedToCompleted = true;
        for (auto prevCompletedDomainItr = prevCompleted.begin(); 
             prevCompletedDomainItr != prevCompleted.end(); 
             ++prevCompletedDomainItr)
        {
            if(*prevCompletedDomainItr == *currentCompletedDomainItr)
            {
                changedToCompleted = false;
                break;
            }
        }

        if (changedToCompleted == true)
        {
            changedCompletedDomains.push_back(*currentCompletedDomainItr);
        }
    }

    //
    // new InProgressDomain if any.
    //
    if (!currentInProgress.empty())
    {
        if (currentInProgress != prevInProgress)
        {
            changedInProgressDomain = currentInProgress;
        }
    }
}

void UpgradeHelper::ToPublicUpgradeDomains(
    __in ScopedHeap & heap,
    wstring const& inProgress,
    vector<wstring> const& pending,
    vector<wstring> const& completed,
    __out ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & array)
{
    size_t inProgressDomainCount = inProgress.empty() ? 0 : 1;
    size_t totalUpgradeDomainCount = inProgressDomainCount + completed.size() + pending.size();

    if (totalUpgradeDomainCount == 0)
    {
        return;
    }

    array = heap.AddArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION>(totalUpgradeDomainCount);
    for (size_t ix = 0; ix < totalUpgradeDomainCount; ++ix)
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION & description = array.GetRawArray()[ix];
        description.State = FABRIC_UPGRADE_DOMAIN_STATE_INVALID;
    }

    if (inProgressDomainCount > 0)
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION & description = array.GetRawArray()[0];

        description.Name = heap.AddString(inProgress);
        description.State = FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS;
    }

    size_t startIndex = inProgressDomainCount;
    for (size_t ix = startIndex; ix < startIndex + completed.size(); ++ix)
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION & description = array.GetRawArray()[ix];

        description.Name = heap.AddString(completed[ix - startIndex]);
        description.State = FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED;
    }

    startIndex = inProgressDomainCount + completed.size();
    for (size_t ix = startIndex; ix < startIndex + pending.size(); ++ix)
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION & description = array.GetRawArray()[ix];

        description.Name = heap.AddString(pending[ix - startIndex]);
        description.State = FABRIC_UPGRADE_DOMAIN_STATE_PENDING;
    }
}

void UpgradeHelper::ToPublicChangedUpgradeDomains(
    __in ScopedHeap & heap,
    wstring const& changedInProgressDomain,
    vector<wstring> const& changedCompletedDomains,
    __out ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & array)
{
    size_t totalResultCount = changedInProgressDomain.empty() ? 0 : 1;
    totalResultCount = totalResultCount + changedCompletedDomains.size();

    if (totalResultCount == 0)
    {
        return;
    }
    
    array = heap.AddArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION>(totalResultCount);

    for (size_t ix = 0; ix < changedCompletedDomains.size(); ++ix)
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION & description = array.GetRawArray()[ix];

        ASSERT_IF(changedCompletedDomains[ix].empty(), "Completed domain cannot be with an empty name");
        description.Name = heap.AddString(changedCompletedDomains[ix]);
        description.State = FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED;
    }

    if (!changedInProgressDomain.empty())
    {
        FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION & description = array.GetRawArray()[totalResultCount - 1];

        description.Name = heap.AddString(changedInProgressDomain);
        description.State = FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS;
    }
}
