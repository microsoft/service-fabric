// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // {79516677-51A1-4468-A0AC-F66A46DE304A}
        static const GUID CLSID_ComReplicator_CatchupReplicaSetOperation = 
            { 0x79516677, 0x51a1, 0x4468, { 0xa0, 0xac, 0xf6, 0x6a, 0x46, 0xde, 0x30, 0x4a } };

        class ComReplicator::CatchupReplicaSetOperation : 
            public ComReplicator::BaseOperation
        {
            DENY_COPY(CatchupReplicaSetOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                CatchupReplicaSetOperation,
                CLSID_ComReplicator_CatchupReplicaSetOperation,
                CatchupReplicaSetOperation,
                BaseOperation)

        public:
            CatchupReplicaSetOperation(
                Replicator & replicator, 
                FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode);

            virtual ~CatchupReplicaSetOperation() { }

            static HRESULT End(__in IFabricAsyncOperationContext * context);

        protected:

            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            friend class ComReplicator;

            void CatchupReplicaSetCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void FinishCatchupReplicaSet(Common::AsyncOperationSPtr const & asyncOperation);

            FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
