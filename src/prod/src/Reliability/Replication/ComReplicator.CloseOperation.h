// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // {DADD232A-56E6-4A34-87AE-661D8BEB014D}
        static const GUID CLSID_ComReplicator_CloseOperation = 
            { 0xdadd232a, 0x56e6, 0x4a34, { 0x87, 0xae, 0x66, 0x1d, 0x8b, 0xeb, 0x1, 0x4d } };
        
        class ComReplicator::CloseOperation : 
            public ComReplicator::BaseOperation
        {
            DENY_COPY(CloseOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                CloseOperation,
                CLSID_ComReplicator_CloseOperation,
                CloseOperation,
                BaseOperation)

        public:
            explicit CloseOperation(Replicator & replicator);

            virtual ~CloseOperation() { }

            static HRESULT End(
                __in IFabricAsyncOperationContext * context);

        protected:

            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            friend class ComReplicator;

            void CloseCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void FinishClose(Common::AsyncOperationSPtr const & asyncOperation);
        };

     } // end namespace ReplicationComponent
} // end namespace Reliability
