// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class WriteServiceAtNameOwnerAsyncOperation 
        : public Common::AsyncOperation
        , public ActivityTracedComponent<Common::TraceTaskCodes::NamingStoreService>
    {
    public:
        WriteServiceAtNameOwnerAsyncOperation(
            Common::NamingUri const & name,
            __in NamingStore & store,
            bool isCreationComplete,
            bool shouldUpdatePsd,
            PartitionedServiceDescriptor const & psd,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);
        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, __out FABRIC_SEQUENCE_NUMBER &);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void StartWriteService(Common::AsyncOperationSPtr const &);
        void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        Common::NamingUri name_;
        NamingStore & store_;
        bool isCreationComplete_;
        bool shouldUpdatePsd_;
        PartitionedServiceDescriptor const & psd_; // Lifetime owned by StoreService::ProcessInnerCreateServiceAsyncOperation
        Common::TimeoutHelper timeoutHelper_;
        FABRIC_SEQUENCE_NUMBER lsn_;
    };
}

