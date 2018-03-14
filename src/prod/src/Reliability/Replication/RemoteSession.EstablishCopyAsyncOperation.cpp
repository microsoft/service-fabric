// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

    using Common::Assert;
    using Common::AsyncCallback;
    using Common::AsyncOperation;
    using Common::AsyncOperationSPtr;
    using Common::ComPointer;
    using Common::ErrorCode;

    RemoteSession::EstablishCopyAsyncOperation::EstablishCopyAsyncOperation(
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        : AsyncOperation(callback, state)
        , context_()
    {
    }

    void RemoteSession::EstablishCopyAsyncOperation::OnStart(Common::AsyncOperationSPtr const & )
    {
    }

    void RemoteSession::EstablishCopyAsyncOperation::ResumeOutsideLock(
        Common::AsyncOperationSPtr const &,
        RemoteSession & parent,
        FABRIC_SEQUENCE_NUMBER replicationStartSeq,
        bool hasPersistedState,
        Transport::SendStatusCallback const & sendStatusCallback)
    {
        // The context given to the service for partial copy.
        // If the state provider has no state, keep the context empty.
        // Otherwise, build it on top of the copy context operations 
        // received from the secondary.
        if (hasPersistedState)
        {
            parent.CreateCopyContext(context_);
        }

        // Sends the replica the StartCopy message that contains the
        // replication start LSN.
        // The message must be sent after the copy context is created,
        // so no CopyContext message is dropped.
        parent.StartCopy(replicationStartSeq, sendStatusCallback);
    }

    ErrorCode RemoteSession::EstablishCopyAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        ComPointer<IFabricOperationDataStream> & context)
    {
        auto casted = AsyncOperation::End<EstablishCopyAsyncOperation>(asyncOperation);
        context = std::move(casted->context_);
        return casted->Error;
    }
}
}
