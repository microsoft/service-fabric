// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // {0E270BE8-D4E0-4E97-9281-71D6375D1F96}
        static const GUID CLSID_ComReplicator_BuildIdleReplicaOperation = 
            { 0xe270be8, 0xd4e0, 0x4e97, { 0x92, 0x81, 0x71, 0xd6, 0x37, 0x5d, 0x1f, 0x96 } };
                
        class ComReplicator::BuildIdleReplicaOperation : 
            public ComReplicator::BaseOperation
        {
            DENY_COPY(BuildIdleReplicaOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                BuildIdleReplicaOperation,
                CLSID_ComReplicator_BuildIdleReplicaOperation,
                BuildIdleReplicaOperation,
                BaseOperation)

        public:
            BuildIdleReplicaOperation(
                Replicator & replicator, 
                FABRIC_REPLICA_INFORMATION const * idleReplica);

            virtual ~BuildIdleReplicaOperation() { }

            static HRESULT End(
                __in IFabricAsyncOperationContext * context);

        protected:

            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            friend class ComReplicator;
            
            void BuildIdleReplicaCallback(Common::AsyncOperationSPtr const & asyncOperation);
            
            void FinishBuildIdleReplica(Common::AsyncOperationSPtr const & asyncOperation);

            ReplicaInformation replica_;
        };
     } // end namespace ReplicationComponent
} // end namespace Reliability
