// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // {0B868400-9A57-4C85-9099-25C777C67E8A}
        static const GUID CLSID_ComReplicator_ChangeRoleOperation = 
            { 0xb868400, 0x9a57, 0x4c85, { 0x90, 0x99, 0x25, 0xc7, 0x77, 0xc6, 0x7e, 0x8a } };
        
        class ComReplicator::ChangeRoleOperation : 
            public ComReplicator::BaseOperation
        {
            DENY_COPY(ChangeRoleOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                ChangeRoleOperation,
                CLSID_ComReplicator_ChangeRoleOperation,
                ChangeRoleOperation,
                BaseOperation)

        public:
            ChangeRoleOperation(
                Replicator & replicator, 
                FABRIC_EPOCH const & epoch, 
                FABRIC_REPLICA_ROLE role);

            virtual ~ChangeRoleOperation() { }

            static HRESULT End(__in IFabricAsyncOperationContext *context);

        protected:

            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            friend class ComReplicator;

            void ChangeRoleCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void FinishChangeRole(Common::AsyncOperationSPtr const & asyncOperation);
                        
            FABRIC_EPOCH epoch_;
            FABRIC_REPLICA_ROLE role_;
        };

     } // end namespace ReplicationComponent
} // end namespace Reliability
