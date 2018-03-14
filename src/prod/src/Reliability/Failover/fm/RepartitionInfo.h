// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class RepartitionInfo : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(RepartitionInfo)
            DENY_COPY_ASSIGNMENT(RepartitionInfo)

        public:

            RepartitionInfo();

            RepartitionInfo(
                RepartitionType::Enum repartitionType,
                std::map<FailoverUnitId, ConsistencyUnitDescription> && added,
                std::map<FailoverUnitId, ConsistencyUnitDescription> && removed);

            RepartitionInfo(RepartitionInfo && other);
            RepartitionInfo & operator=(RepartitionInfo && other);

            __declspec(property(get = get_RepartitionType, put = set_RepartitionType)) RepartitionType::Enum RepartitionType;
            RepartitionType::Enum get_RepartitionType() const { return repartitionType_; }
            void set_RepartitionType(RepartitionType::Enum value) { repartitionType_ = value; }

            __declspec(property(get = get_Added)) std::map<FailoverUnitId, ConsistencyUnitDescription> const& Added;
            std::map<FailoverUnitId, ConsistencyUnitDescription> const& get_Added() const { return added_; }

            __declspec(property(get = get_Removed)) std::map<FailoverUnitId, ConsistencyUnitDescription> const& Removed;
            std::map<FailoverUnitId, ConsistencyUnitDescription> const& get_Removed() const { return removed_; }

            bool IsRemoved(FailoverUnitId const& failoverUnitId) const;

            FABRIC_FIELDS_03(repartitionType_, added_, removed_)

        private:

            RepartitionType::Enum repartitionType_;

            // TODO, MMohsin: Consider making these unordered maps
            std::map<FailoverUnitId, ConsistencyUnitDescription> added_;
            std::map<FailoverUnitId, ConsistencyUnitDescription> removed_;
        };
    }
}

DEFINE_USER_MAP_UTILITY(Reliability::FailoverUnitId, Reliability::ConsistencyUnitDescription);
