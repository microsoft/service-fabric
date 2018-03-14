// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class RemoteSession::EstablishCopyAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(EstablishCopyAsyncOperation)

        public:
            EstablishCopyAsyncOperation(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            virtual ~EstablishCopyAsyncOperation() {}

            void ResumeOutsideLock(
                Common::AsyncOperationSPtr const & thisSPtr,
                RemoteSession & parent,
                FABRIC_SEQUENCE_NUMBER replicationStartSeq,
                bool hasPersistedState,
                Transport::SendStatusCallback const & sendStatusCallback);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                Common::ComPointer<IFabricOperationDataStream> & context);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            Common::ComPointer<IFabricOperationDataStream> context_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
