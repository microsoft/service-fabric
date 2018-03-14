// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataClusterUpgradeStateSnapshot : public Store::StoreData
        {
            DENY_COPY(StoreDataClusterUpgradeStateSnapshot)

        public:
            StoreDataClusterUpgradeStateSnapshot();
            StoreDataClusterUpgradeStateSnapshot(StoreDataClusterUpgradeStateSnapshot &&);
            explicit StoreDataClusterUpgradeStateSnapshot(Management::HealthManager::ClusterUpgradeStateSnapshot &&);

            __declspec(property(get=get_StateSnapshot)) Management::HealthManager::ClusterUpgradeStateSnapshot & StateSnapshot;
            Management::HealthManager::ClusterUpgradeStateSnapshot & get_StateSnapshot() { return stateSnapshot_; }

            virtual ~StoreDataClusterUpgradeStateSnapshot() { }
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_01(stateSnapshot_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            Management::HealthManager::ClusterUpgradeStateSnapshot stateSnapshot_;
        };
    }
}
