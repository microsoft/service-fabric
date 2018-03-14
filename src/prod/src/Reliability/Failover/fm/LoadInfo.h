// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class LoadInfo : public StoreData
        {
            DENY_COPY_ASSIGNMENT(LoadInfo);

        public:
            LoadInfo();

            LoadInfo(LoadBalancingComponent::LoadOrMoveCostDescription && loads);

            LoadInfo(LoadInfo && other);

            LoadInfo(LoadInfo const& other);

            LoadInfo & operator = (LoadInfo && other);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            __declspec (property(get=get_LoadDescription)) LoadBalancingComponent::LoadOrMoveCostDescription const& LoadDescription;
            LoadBalancingComponent::LoadOrMoveCostDescription const& get_LoadDescription() const { return loadDescription_; }

            __declspec (property(get=get_IdString)) std::wstring const& IdString;
            std::wstring const& get_IdString() const;

            void AdjustTimestamps(Common::TimeSpan diff);
            bool MergeLoads(LoadBalancingComponent::LoadOrMoveCostDescription && loads);
            bool MarkForDeletion();

            LoadBalancingComponent::LoadOrMoveCostDescription GetPLBLoadOrMoveCostDescription();

            void StartPersist();
            bool OnPersistCompleted(bool isSuccess, bool isDeleted, LoadBalancingComponent::IPlacementAndLoadBalancing & plb);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            FABRIC_FIELDS_01(loadDescription_);

        private:
            bool OnUpdate(bool changed);

            LoadBalancingComponent::LoadOrMoveCostDescription loadDescription_;

            mutable std::wstring idString_;

            bool isPending_;
            bool isUpdating_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Reliability::FailoverManagerComponent::LoadInfo);
  
