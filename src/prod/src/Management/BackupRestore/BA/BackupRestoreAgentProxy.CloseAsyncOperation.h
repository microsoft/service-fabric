// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupRestoreAgentProxy::CloseAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(CloseAsyncOperation);

        public:
            CloseAsyncOperation(
                BackupRestoreAgentProxy & owner,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) :
                owner_(owner),
                outstandingOperations_(1),
                AsyncOperation(callback, parent) {}
            virtual ~CloseAsyncOperation() {}

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            BackupRestoreAgentProxy & owner_;
            Common::atomic_uint64 outstandingOperations_;
        };
    }
}
