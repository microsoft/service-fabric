// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::OpenAsyncOperation 
        : public Common::AsyncOperation
        , public PartitionedReplicaTraceComponent
    {
    public:
        OpenAsyncOperation(PartitionedReplicaTraceComponent const &, Common::AsyncCallback const &, Common::AsyncOperationSPtr const &);
        OpenAsyncOperation(PartitionedReplicaTraceComponent const &, Common::ErrorCode const &, Common::AsyncCallback const &, Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

    protected:

        virtual void OnStart(Common::AsyncOperationSPtr const &) override;
        virtual void OnCancel() override;
        virtual void OnCompleted() override;

    private:

        Common::ErrorCode openError_;
        Common::ManualResetEvent completionEvent_;
    };
}
