// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        struct UnhealthyState
            : public Serialization::FabricSerializable
        {
        public:
            UnhealthyState();
            UnhealthyState(ULONG errorCount, ULONG totalCount);

            bool operator == (UnhealthyState const & other) const;
            bool operator != (UnhealthyState const & other) const;

            float GetUnhealthyPercent() const;
            static float GetUnhealthyPercent(ULONG errorCount, ULONG totalCount);

            FABRIC_FIELDS_02(ErrorCount, TotalCount)

            ULONG ErrorCount;
            ULONG TotalCount;
        };

        class ClusterUpgradeStateSnapshot
            : public Serialization::FabricSerializable
        {
            DENY_COPY(ClusterUpgradeStateSnapshot)

        public:
            // Type that defines health per upgrade domain.
            typedef std::pair<std::wstring, UnhealthyState> UpgradeDomainStatusPair;
            typedef std::map<std::wstring, UnhealthyState> UpgradeDomainStatusMap;

            ClusterUpgradeStateSnapshot();
            virtual ~ClusterUpgradeStateSnapshot();

            ClusterUpgradeStateSnapshot(ClusterUpgradeStateSnapshot && other);
            ClusterUpgradeStateSnapshot & operator = (ClusterUpgradeStateSnapshot && other);

            __declspec(property(get = get_GlobalUnhealthyState)) UnhealthyState const & GlobalUnhealthyState;
            UnhealthyState const & get_GlobalUnhealthyState() const { return globalUnhealthyState_; }
            
            __declspec(property(get = get_UpgradeDomainUnhealthyStates)) UpgradeDomainStatusMap const & UpgradeDomainUnhealthyStates;
            UpgradeDomainStatusMap const & get_UpgradeDomainUnhealthyStates() const { return upgradeDomainUnhealthyStates_; }

            bool IsValid() const;

            void SetGlobalState(ULONG errorCount, ULONG totalCount);

            void AddUpgradeDomainEntry(
                std::wstring const & upgradeDomainName,
                ULONG errorCount,
                ULONG totalCount);

            bool HasUpgradeDomainEntry(std::wstring const & upgradeDomainName) const;

            // Returns the percent associated with the upgrade domain name if an entry in the map exists
            // or error otherwise
            Common::ErrorCode TryGetUpgradeDomainEntry(
                std::wstring const & upgradeDomainName,
                __inout ULONG & errorCount,
                __inout ULONG & totalCount) const;

            bool IsGlobalDeltaRespected(
                ULONG errorCount,
                ULONG totalCount,
                BYTE maxPercentDelta) const;
            
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
                        
            FABRIC_FIELDS_02(globalUnhealthyState_, upgradeDomainUnhealthyStates_)

            static bool IsDeltaRespected(
                ULONG baselineErrorCount,
                ULONG baselineTotalCount,
                ULONG errorCount,
                ULONG totalCount,
                BYTE maxPercentDelta);

        private:
            UnhealthyState globalUnhealthyState_;
            UpgradeDomainStatusMap upgradeDomainUnhealthyStates_;
        };
    }
}

DEFINE_USER_MAP_UTILITY(std::wstring, Management::HealthManager::UnhealthyState);
