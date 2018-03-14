// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // {28A4A373-1AAE-4D68-9CED-61C228076A45}
        static const GUID CLSID_ComReplicator_OnDataLossOperation = 
            { 0x28a4a373, 0x1aae, 0x4d68, { 0x9c, 0xed, 0x61, 0xc2, 0x28, 0x7, 0x6a, 0x45 } };
        class ComReplicator::OnDataLossOperation : 
            public ComReplicator::BaseOperation
        {
            DENY_COPY(OnDataLossOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                OnDataLossOperation,
                CLSID_ComReplicator_OnDataLossOperation,
                OnDataLossOperation,
                BaseOperation)

        public:
            explicit OnDataLossOperation(Replicator & replicator);

            virtual ~OnDataLossOperation() { }

            static HRESULT End(
                __in IFabricAsyncOperationContext * context,
                __out BOOLEAN * isStateChanged);

        protected:

            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            friend class ComReplicator;

            void OnDataLossCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void FinishOnDataLoss(Common::AsyncOperationSPtr const & asyncOperation);

            BOOLEAN isStateChanged_;
        };
     } // end namespace ReplicationComponent
} // end namespace Reliability
