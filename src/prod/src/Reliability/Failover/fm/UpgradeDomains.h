// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class UpgradeDomains
        {
            DENY_COPY_ASSIGNMENT(UpgradeDomains)

        public:

            UpgradeDomains(UpgradeDomainSortPolicy::Enum sortPolicy);

            UpgradeDomains(UpgradeDomains const& other, UpgradeDomainSortPolicy::Enum sortPolicy);

            UpgradeDomains(UpgradeDomains const& other);

            __declspec(property(get = get_UDs)) std::vector<std::wstring> const& UDs;
            std::vector<std::wstring> const& get_UDs() const { return upgradeDomains_; }

            __declspec(property(get = get_IsStale, put = set_IsStale)) bool IsStale;
            bool get_IsStale() const { return isStale_; }
            void set_IsStale(bool value) { isStale_ = value; }

            bool Exists(std::wstring const& upgradeDomain) const;

            void AddUpgradeDomain(std::wstring const& upgradeDomain);
            void RemoveUpgradeDomain(std::wstring const& upgradeDomain);

            size_t GetUpgradeDomainIndex(std::wstring const& upgradeDomain) const;
            std::wstring GetUpgradeDomain(size_t upgradeDomainIndex) const;

            bool IsUpgradeComplete(std::wstring const& currentUpgradeDomain) const;

            bool IsUpgradeComplete(size_t upgradeDomainIndex) const;

            std::set<std::wstring> GetCompletedUpgradeDomains(size_t currentUpgradeDomainIndex) const;

            void GetProgress(
                size_t upgradeDomainIndex,
                _Out_ std::vector<std::wstring> & completedUDs,
                _Out_ std::vector<std::wstring> & pendingUDs) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        private:

            std::vector<std::wstring> upgradeDomains_;
            UpgradeDomainSortPolicy::Enum sortPolicy_;

            bool isStale_;
        };
    }
}
