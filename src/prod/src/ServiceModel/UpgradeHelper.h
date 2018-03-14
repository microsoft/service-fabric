// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class UpgradeHelper 
    {
        public: // special-case TimeSpan::MaxValue handling for rollback

            static Common::TimeSpan GetReplicaSetCheckTimeoutForInitialUpgrade(DWORD wrapperTimeout);
            static Common::TimeSpan GetReplicaSetCheckTimeoutForModify(Common::TimeSpan const);
            static DWORD ToPublicReplicaSetCheckTimeoutInSeconds(Common::TimeSpan const);

        public: // building upgrade progress results
            
            static void GetChangedUpgradeDomains(
                std::wstring const& currentInProgress,
                std::vector<std::wstring> const& currentCompleted,
                std::wstring const& prevInProgress,
                std::vector<std::wstring> const& prevCompleted,
                __out std::wstring &changedInProgressDomain,
                __out std::vector<std::wstring> &changedCompletedDomains);

            static void ToPublicUpgradeDomains(
                __in Common::ScopedHeap &,
                std::wstring const& inProgress,
                std::vector<std::wstring> const& pending,
                std::vector<std::wstring> const& completed,
                __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &);

            static void ToPublicChangedUpgradeDomains(
                __in Common::ScopedHeap &,
                std::wstring const& changedInProgressDomain,
                std::vector<std::wstring> const& changedCompletedDomains,
                __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &);
    };
}
