// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IReplicator
    {
        Common::AsyncOperationSPtr BeginOpen(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const & asyncOperation,  
            __out std::wstring & replicationEndpoint);

        Common::AsyncOperationSPtr BeginChangeRole(
            ::FABRIC_EPOCH const & epoch,
            ::FABRIC_REPLICA_ROLE newRole,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);       

        Common::ErrorCode EndChangeRole(
            Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginUpdateEpoch( 
            ::FABRIC_EPOCH const & epoch,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndUpdateEpoch(
            Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginClose(
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndClose(
            Common::AsyncOperationSPtr const & asyncOperation);

        void Abort();

        Common::ErrorCode GetStatus(
            __out FABRIC_SEQUENCE_NUMBER & firstLsn, 
            __out FABRIC_SEQUENCE_NUMBER & lastLsn);
    };
}
