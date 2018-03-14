// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator::OpenAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(OpenAsyncOperation)

        public:
            OpenAsyncOperation(
                __in Replicator & parent,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation, 
                __out std::wstring & endpoint);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            Replicator & parent_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
