// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class RemoveServiceAtAuthorityAsyncOperation 
        : public Common::AsyncOperation
        , public ActivityTracedComponent<Common::TraceTaskCodes::NamingStoreService>
    {
    public:
        RemoveServiceAtAuthorityAsyncOperation(
            Common::NamingUri const & name,
            __in NamingStore & store,
            bool isDeletionComplete,
            bool isForceDelete,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);
        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, __out bool & isForceDelete);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void StartRemoveService(Common::AsyncOperationSPtr const &);
        void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        Common::NamingUri name_;
        NamingStore & store_;
        bool isDeletionComplete_;
        bool isForceDelete_;
        Common::TimeoutHelper timeoutHelper_;
    };
}
