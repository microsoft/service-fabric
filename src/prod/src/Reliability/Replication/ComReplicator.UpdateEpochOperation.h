// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // {7D94F72A-8ED9-42D1-9A65-C1BA4DB28B5C}
        static const GUID CLSID_ComReplicator_UpdateEpochOperation = 
            { 0x7d94f72a, 0x8ed9, 0x42d1, { 0x9a, 0x65, 0xc1, 0xba, 0x4d, 0xb2, 0x8b, 0x5c } };
        
        class ComReplicator::UpdateEpochOperation : 
            public ComReplicator::BaseOperation
        {
            DENY_COPY(UpdateEpochOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                UpdateEpochOperation,
                CLSID_ComReplicator_UpdateEpochOperation,
                UpdateEpochOperation,
                BaseOperation)

        public:
            UpdateEpochOperation(
                Replicator & replicator, 
                FABRIC_EPOCH const & epoch);

            virtual ~UpdateEpochOperation() { }

            static HRESULT End(__in IFabricAsyncOperationContext *context);

        protected:

            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            friend class ComReplicator;

            void UpdateEpochCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void FinishUpdateEpoch(Common::AsyncOperationSPtr const & asyncOperation);
                        
            FABRIC_EPOCH epoch_;
        };

     } // end namespace ReplicationComponent
} // end namespace Reliability
