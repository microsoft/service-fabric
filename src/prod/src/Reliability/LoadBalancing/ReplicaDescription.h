// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ReplicaRole.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ReplicaDescription
        {
            DENY_COPY_ASSIGNMENT(ReplicaDescription);

        public:
            ReplicaDescription(
                Federation::NodeInstance nodeInstance,
                ReplicaRole::Enum role,
                Reliability::ReplicaStates::Enum state,
                bool isUp,
                Reliability::ReplicaFlags::Enum flags = Reliability::ReplicaFlags::None);

            ReplicaDescription(ReplicaDescription const & other);

            ReplicaDescription(ReplicaDescription && other);

            ReplicaDescription & operator = (ReplicaDescription && other);

            __declspec (property(get=get_NodeId)) Federation::NodeId NodeId;
            Federation::NodeId get_NodeId() const { return nodeInstance_.Id; }

            __declspec (property(get=get_NodeInstance)) Federation::NodeInstance NodeInstance;
            Federation::NodeInstance get_NodeInstance() const { return nodeInstance_; }

            __declspec (property(get = get_IsUp)) bool IsUp;
            bool get_IsUp() const { return isUp_; }

            __declspec (property(get=get_IsInTransition)) bool IsInTransition;
            bool get_IsInTransition() const { return isInTransition_; }

            __declspec (property(get = get_MakesFuInTransition)) bool MakesFuInTransition;
            bool get_MakesFuInTransition() const { return makesFuInTransition_; }

            __declspec (property(get = get_IsStandBy)) bool IsStandBy;
            bool get_IsStandBy() const { return state_ == ReplicaStates::StandBy; }

            __declspec (property(get=get_IsInBuild)) bool IsInBuild;
            bool get_IsInBuild() const { return state_ == ReplicaStates::InBuild; }

            __declspec (property(get = get_IsReady)) bool IsReady;
            bool get_IsReady() const { return state_ == ReplicaStates::Ready; }

            __declspec (property(get = get_IsDropped)) bool IsDropped;
            bool get_IsDropped() const { return state_ == ReplicaStates::Dropped; }

            __declspec (property(get = get_CurrentRole)) ReplicaRole::Enum CurrentRole;
            ReplicaRole::Enum get_CurrentRole() const { return role_; }

            __declspec (property(get = get_CurrentState)) ReplicaStates::Enum CurrentState;
            ReplicaStates::Enum get_CurrentState() const { return state_; }

            __declspec (property(get = get_IsToBeDropped)) bool IsToBeDropped;
            bool get_IsToBeDropped() const { return IsToBeDroppedByFM || IsToBeDroppedByPLB || IsToBeDroppedForNodeDeactivation; }

            __declspec (property(get = get_IsOffline)) bool IsOffline;
            bool get_IsOffline() const { return (!IsUp && !IsDropped); }

            __declspec (property(get = get_HasAnyFlag)) bool HasAnyFlag;
            bool get_HasAnyFlag() const { return flags_ != Reliability::ReplicaFlags::None; }

            __declspec (property(get = get_IsToBeDroppedByFM)) bool IsToBeDroppedByFM;
            bool get_IsToBeDroppedByFM() const { return (flags_ & Reliability::ReplicaFlags::ToBeDroppedByFM) != 0; }

            __declspec (property(get = get_IsToBeDroppedByPLB)) bool IsToBeDroppedByPLB;
            bool get_IsToBeDroppedByPLB() const { return (flags_ & Reliability::ReplicaFlags::ToBeDroppedByPLB) != 0; }

            __declspec (property(get = get_IsToBeDroppedForNodeDeactivation)) bool IsToBeDroppedForNodeDeactivation;
            bool get_IsToBeDroppedForNodeDeactivation() const { return (flags_ & Reliability::ReplicaFlags::ToBeDroppedForNodeDeactivation) != 0; }

            __declspec (property(get = get_IsToBePromoted)) bool IsToBePromoted;
            bool get_IsToBePromoted() const { return (flags_ & Reliability::ReplicaFlags::ToBePromoted) != 0; }

            __declspec (property(get = get_IsPendingRemove)) bool IsPendingRemove;
            bool get_IsPendingRemove() const { return (flags_ & Reliability::ReplicaFlags::PendingRemove) != 0; }

            __declspec (property(get = get_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return (flags_ & Reliability::ReplicaFlags::Deleted) != 0; }

            __declspec (property(get = get_IsPreferredPrimaryLocation)) bool IsPreferredPrimaryLocation;
            bool get_IsPreferredPrimaryLocation() const { return (flags_ & Reliability::ReplicaFlags::PreferredPrimaryLocation) != 0; }

            __declspec (property(get = get_IsPreferredReplicaLocation)) bool IsPreferredReplicaLocation;
            bool get_IsPreferredReplicaLocation() const { return (flags_ & Reliability::ReplicaFlags::PreferredReplicaLocation) != 0; }

            __declspec (property(get = get_IsPrimaryToBeSwappedOut)) bool IsPrimaryToBeSwappedOut;
            bool get_IsPrimaryToBeSwappedOut() const { return (flags_ & Reliability::ReplicaFlags::PrimaryToBeSwappedOut) != 0; }

            __declspec (property(get = get_IsPrimaryToBePlaced)) bool IsPrimaryToBePlaced;
            bool get_IsPrimaryToBePlaced() const { return (flags_ & Reliability::ReplicaFlags::PrimaryToBePlaced) != 0; }

            __declspec (property(get = get_IsReplicaToBePlaced)) bool IsReplicaToBePlaced;
            bool get_IsReplicaToBePlaced() const { return (flags_ & Reliability::ReplicaFlags::ReplicaToBePlaced) != 0; }

            __declspec (property(get = get_IsMoveInProgress)) bool IsMoveInProgress;
            bool get_IsMoveInProgress() const { return (flags_ & Reliability::ReplicaFlags::MoveInProgress) != 0; }

            __declspec (property(get = get_IsEndpointAvailable)) bool IsEndpointAvailable;
            bool get_IsEndpointAvailable() const { return (flags_ & Reliability::ReplicaFlags::EndpointAvailable) != 0; }

            __declspec (property(get = get_ShouldDisappear)) bool ShouldDisappear;
            bool get_ShouldDisappear() const { return IsMoveInProgress || IsToBeDropped; }

            __declspec (property(get = get_IsInUpgrade)) bool IsInUpgrade;
            bool get_IsInUpgrade() const { return IsPrimaryToBeSwappedOut || IsPrimaryToBePlaced || IsReplicaToBePlaced; };

            __declspec (property(get = get_VerifyReplicaLoad)) std::vector<uint> & VerifyReplicaLoad;
            std::vector<uint> & get_VerifyReplicaLoad() const { return verifyReplicaLoad_; };

            bool UsePrimaryLoad() const;
            bool UseSecondaryLoad() const;

            // Check if replica is None and if it has resource governance configured
            bool UseNoneLoad(bool isRGLoad) const;

            bool HasLoad() const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            // check if replica could carry reservation or participate to application scaleout
            bool CouldCarryReservationOrScaleout() const;

        private:
            Federation::NodeInstance nodeInstance_;
            ReplicaRole::Enum role_;
            ReplicaStates::Enum state_;

            bool isUp_;

            // whether the replica is ToBePromoted, MoveInProgress, ToBeDropped, InBuild, StandBy, Offline
            bool isInTransition_;

            // whether the replica is active (up and non-StandBy) ToBePromoted or InBuild - FT should be marked as in transition
            bool makesFuInTransition_;

            Reliability::ReplicaFlags::Enum flags_;

            //Verify replica load added on node
            mutable std::vector<uint> verifyReplicaLoad_;
        };
    }
}
