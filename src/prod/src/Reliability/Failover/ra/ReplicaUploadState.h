// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        /*
            This class keeps track of whether the replica for a partition has been
            uploaded to the FM as part of the initial upload of all replicas

            The replica upload pending is marked during node up ack processing when
            it is determined whether to actually upload this replica or not. The 
            upload is only marked as pending. The actual upload may be delayed 
            until the reopen success wait interval has expired or immedate.

            Newly created replicas (due to the FM request) are never uploaded

            The upload is completed when the FM sends a message for this partition

            The FMMessageState is owned by the FT 
         */
        class ReplicaUploadState
        {
        public:
            ReplicaUploadState(FMMessageState * fmMessageState, FailoverManagerId const & owner);
            ReplicaUploadState(FMMessageState * fmMessageState, ReplicaUploadState const & other);
            ReplicaUploadState & operator=(ReplicaUploadState const & other);

            __declspec(property(get = get_IsUploadPending)) bool IsUploadPending;
            bool get_IsUploadPending() const { return replicaUploadPending_.IsSet; }

            __declspec(property(get = get_ReplicaUploadPending)) Infrastructure::SetMembershipFlag const & ReplicaUploadPending;
            Infrastructure::SetMembershipFlag const & get_ReplicaUploadPending() const { return replicaUploadPending_; }
            Infrastructure::SetMembershipFlag & Test_GetSet() { return replicaUploadPending_; }

            void MarkUploadPending(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void UploadReplica(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnUploaded(
                Infrastructure::StateUpdateContext & stateUpdateContext);

        private:
            FMMessageState * fmMessageState_;
            Infrastructure::SetMembershipFlag replicaUploadPending_;
            bool uploadRequested_;
        };
    }
}



