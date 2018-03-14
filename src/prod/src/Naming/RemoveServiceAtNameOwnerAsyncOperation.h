// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class RemoveServiceAtNameOwnerAsyncOperation 
        : public Common::AsyncOperation
        , public ActivityTracedComponent<Common::TraceTaskCodes::NamingStoreService>
    {
    public:
        RemoveServiceAtNameOwnerAsyncOperation(
            Common::NamingUri const & name,
            __in NamingStore & store,
            bool isDeletionComplete,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);
        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, __out FABRIC_SEQUENCE_NUMBER & lsn);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void StartRemoveService(Common::AsyncOperationSPtr const &);
        void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        Common::NamingUri name_;
        NamingStore & store_;
        bool isDeletionComplete_;
        Common::TimeoutHelper timeoutHelper_;
        FABRIC_SEQUENCE_NUMBER lsn_;
    };
}
