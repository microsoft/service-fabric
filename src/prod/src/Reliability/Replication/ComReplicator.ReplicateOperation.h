// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        static const GUID CLSID_ComReplicator_ReplicateOperation = 
            { 0x2f18227c, 0xda2f, 0x4be7, { 0xaa, 0x94, 0xc6, 0xa, 0x1f, 0x97, 0xb6, 0x1d } };

        class ComReplicator::ReplicateOperation : 
            public ComReplicator::BaseOperation
        {
            DENY_COPY(ReplicateOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                ReplicateOperation,
                CLSID_ComReplicator_ReplicateOperation,
                ReplicateOperation,
                BaseOperation)

        public:
            ReplicateOperation(
                Replicator & replicator,
                Common::ComPointer<IFabricOperationData> && operation);

            virtual ~ReplicateOperation() { }

            __declspec (property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER SequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_SequenceNumber() const { return sequenceNumber_; }

            static HRESULT End(
                __in IFabricAsyncOperationContext * context,
                __out FABRIC_SEQUENCE_NUMBER * sequenceNumber);

        protected:

            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            friend class ComReplicator;

            void ReplicateCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void FinishReplicate(Common::AsyncOperationSPtr const & asyncOperation);

            Common::ComPointer<IFabricOperationData> comOperationPointer_;
            FABRIC_SEQUENCE_NUMBER sequenceNumber_;
        };
    }
}
