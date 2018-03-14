// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        static const GUID CLSID_ComReplicator_OpenOperation = 
            { 0xb4c82a16, 0x6d72, 0x4c23, { 0x94, 0x4e, 0x72, 0x4d, 0x16, 0xd3, 0x9d, 0x6a } };

        class ComReplicator::OpenOperation : 
            public ComReplicator::BaseOperation
        {
            DENY_COPY(OpenOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                OpenOperation,
                CLSID_ComReplicator_OpenOperation,
                OpenOperation,
                BaseOperation)

        public:
            explicit OpenOperation(__in Replicator & replicator);

            virtual ~OpenOperation() { }

            static HRESULT End(
                __in IFabricAsyncOperationContext *context, 
                __out IFabricStringResult **replicationEndpoint);

        protected:

            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            friend class ComReplicator;

            void OpenCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void FinishOpen(Common::AsyncOperationSPtr const & asyncOperation);

            std::wstring replicationEndpoint_;
        };

     } // end namespace ReplicationComponent
} // end namespace Reliability
