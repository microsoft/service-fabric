// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IPrimaryReplicator
    {
        Common::ErrorCode UpdateCatchUpReplicaSetConfiguration(
            ::FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration,
            ::FABRIC_REPLICA_SET_CONFIGURATION const * previousConfiguration);

        Common::AsyncOperationSPtr BeginWaitForCatchUpQuorum(
            ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndWaitForCatchUpQuorum(
            Common::AsyncOperationSPtr const & asyncOperation);

        Common::ErrorCode UpdateCurrentReplicaSetConfiguration(
            ::FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration);

        Common::AsyncOperationSPtr BeginBuildIdleReplica( 
            ::FABRIC_REPLICA_INFORMATION const * replica,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndBuildIdleReplica(
            Common::AsyncOperationSPtr const & asyncOperation);       

        Common::AsyncOperationSPtr BeginOnDataLoss( 
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndOnDataLoss(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out bool & isStateChanged);     

        Common::ErrorCode RemoveIdleReplica(
            FABRIC_REPLICA_ID replicaId);
    };
}
