// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        namespace PLBFailoverUnitDescriptionFlags
        {
            struct Flags
            {
            public:
                enum Enum
                {
                    None = 0x00,
                    Deleted = 0x01,
                    InTransition = 0x02,
                    QuorumLost = 0x04,
                    ReconfigurationInProgress = 0x08,
                    InTransitionReplica = 0x10,
                    InUpgradeReplica = 0x20,
                    ToBePromotedReplica = 0x40
                };

                Flags(
                    bool deleted = false,
                    bool inTransition = false,
                    bool quorumLost = false,
                    bool reconfigurationInProgress = false,
                    bool inTransitionReplica = false,
                    bool inUpgradeReplica = false,
                    bool toBePromotedReplica = false);

                void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

                __declspec (property(get = get_Value)) int Value;
                int get_Value() const { return value_; }

                bool IsDeleted() const { return ((value_ & Deleted) != 0); }
                void SetDeleted(bool deleted) { deleted ? value_ |= Deleted : value_ &= (~Deleted); }

                bool IsInTransition() const { return ((value_ & InTransition) != 0); }
                void SetInTransition(bool inTransition) { inTransition ? value_ |= InTransition : value_ &= (~InTransition); }

                bool IsInQuorumLost() const { return ((value_ & QuorumLost) != 0); }
                void SetInQuorumLost(bool inQuorumLost) { inQuorumLost ? value_ |= QuorumLost : value_ &= (~QuorumLost); }

                bool IsReconfigurationInProgress() const { return ((value_ & ReconfigurationInProgress) != 0); }
                void SetReconfigurationInProgress(bool reconfigurationInProgress) { reconfigurationInProgress ? value_ |= ReconfigurationInProgress : value_ &= (~ReconfigurationInProgress); }

                bool HasInTransitionReplica() const { return ((value_ & InTransitionReplica) != 0); }
                void SetHasInTransitionReplica(bool hasInTransitionReplica) { hasInTransitionReplica ? value_ |= InTransitionReplica : value_ &= (~InTransitionReplica); }

                bool HasInUpgradeReplica() const { return ((value_ & InUpgradeReplica) != 0); }
                void SetHasInUpgradeReplica(bool hasInUpgradeReplica) { hasInUpgradeReplica ? value_ |= InUpgradeReplica : value_ &= (~InUpgradeReplica); }

                bool HasToBePromotedReplica() const { return ((value_ & ToBePromotedReplica) != 0); }
                void SetHasToBePromotedReplica(bool hasToBePromotedReplica) { hasToBePromotedReplica ? value_ |= ToBePromotedReplica : value_ &= (~ToBePromotedReplica); }

            private:

                int value_;
            };
        }
    }
}
